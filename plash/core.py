import atexit
import errno
import hashlib
import os
import re
import shlex
import shutil
import subprocess
import sys
import tarfile
from os import path
from os.path import abspath, join
from tempfile import mkdtemp
from urllib.request import urlopen

from .utils import deescalate_sudo, die, hashstr, info, run

BASE_DIR = os.environ.get('PLASH_DATA', '/var/lib/plash')
TMP_DIR = join(BASE_DIR, 'tmp')
BUILDS_DIR = join(BASE_DIR, 'builds')
LINKS_DIR = join(BASE_DIR, 'links')


class BuildError(Exception):
    pass


class ContainerDoesNotExist(Exception):
    pass


def umount(mountpoint):
    subprocess.check_call(['umount', '--lazy', '--recursive', mountpoint])


def ensure_data_dirs():
    # create the basic directory structure
    try:
        os.mkdir(BASE_DIR, 0o0700)
    except FileExistsError:
        return
    for dir in [BUILD_DIR, TMP_DIR, LINKS_DIR]:
        try:
            os.mkdir(dir)
        except FileExistsError:
            pass


class Container:
    '''
    alias = symlinked name
    lname = layer (path) name

    '''

    def __init__(self, layers):
        assert isinstance(layers, list), repr(layers)
        assert layers
        for layer in layers:
            assert not layer.endswith('.'), layers
        self.layers = layers

    @classmethod
    def by_alias(cls, alias):
        error = False
        try:
            last_layer_path = os.readlink(join(LINKS_DIR, alias))
        except FileNotFoundError:
            error = True
        else:
            # assert not last_layer_path.startswith('/')
            abs_last_layer_path = path.abspath(
                path.join(LINKS_DIR, last_layer_path))
            if not os.path.exists(abs_last_layer_path):
                error = True
        if error:
            raise ContainerDoesNotExist('no such container: {}'.format(alias))
        layers = last_layer_path[len('../builds/'):].split('/children/')
        return cls(layers)

    @classmethod
    def by_node_path(cls, node_path):
        layers = node_path[len(BUILDS_DIR) + 1:].split('/children/')  # brittle
        return cls(layers)

    @property
    def alias(self):
        return self.layers[-1]

    def register_alias(self):
        alias = self.alias
        try:
            os.symlink(
                self.get_node_path(relative=True), join(LINKS_DIR, alias))
        except FileExistsError:
            pass
        return alias

    def unregister_alias(self):
        os.unlink(join(LINKS_DIR, self.alias))

    def _get_child_path(self, cmd):
        layer_hash = self._hash_cmd(cmd.encode())
        last_layer = self.get_node_path()
        return join(last_layer, 'children', layer_hash)

    def _hash_cmd(self, cmd):
        return hashstr((':'.join(self.layers) + ':' + cmd.decode()).encode())

    def get_node_path(self, relative=False):
        p = self._get_layer_paths()[-1]
        if not relative:
            return p
        return '../builds/' + '/children/'.join(self.layers)  # very brittle

    def _get_layer_paths(self):
        lp = [join(BUILDS_DIR, self.layers[0])]
        for ci in self.layers[1:]:
            lp.append(lp[-1] + '/children/' + ci)
        return lp

    def log_access(self):
        for path in reversed(self._get_layer_paths()):
            os.utime(path, None)

    def _prepare_chroot(self, mountpoint):
        run(['mount', '-t', 'proc', 'proc', join(mountpoint, 'proc')])
        run(['mount', '--bind', '/home', join(mountpoint, 'home')])
        run(['mount', '--bind', '/sys', join(mountpoint, 'sys')])
        run(['mount', '--bind', '/dev', join(mountpoint, 'dev')])
        run(['mount', '-t', 'tmpfs', 'tmpfs', join(mountpoint, 'tmp')])
        run([
            'mount', '--bind', '/etc/resolv.conf',
            join(mountpoint, 'etc/resolv.conf')
        ])

    def mount_rootfs(self, *, mountpoint, write_dir=None, workdir=None):
        if write_dir == None:
            write_dir = mkdtemp(dir=TMP_DIR)
        if workdir == None:
            workdir = mkdtemp(dir=TMP_DIR)

        # use the symlinks and not the full paths because the arg size is limited
        # and this is shorter. on my setup i get 58 layers before an error, we could have multiple mount calls to overcome this
        symlinked_layer_paths = []
        layers = self.layers.copy()
        ls = []
        while layers:
            ls.insert(0, layers.copy())
            layers.pop()
        for l in ls:
            symlinked_layer_paths.append(join(LINKS_DIR, Container(l).alias))

        cmd = [
            'mount', '-t', 'overlay', 'overlay', '-o',
            'upperdir={write_dir},lowerdir={dirs},workdir={workdir}'.format(
                write_dir=write_dir,
                workdir=workdir,
                dirs=':'.join(
                    join(p, 'payload')
                    for p in reversed(symlinked_layer_paths))), mountpoint
        ]
        # assert 0, cmd
        run(cmd)

    def build_layer(self, cmd, quiet=False):
        new_child = mkdtemp(dir=TMP_DIR)
        mountpoint = mkdtemp(dir=TMP_DIR)
        new_layer = join(new_child, 'payload')
        os.mkdir(new_layer)
        os.mkdir(join(new_child, 'children'))

        self.mount_rootfs(mountpoint=mountpoint, write_dir=new_layer)

        self._prepare_chroot(mountpoint)

        def preexec_fn():
            os.chroot(mountpoint)
            os.chdir("/")
            os.close(
                0
            )  # close stdin for more reproducible builds - if that does not work well, there is another way

        if quiet:
            out = subprocess.DEVNULL
        else:
            out = 2  # stderr
        p = subprocess.Popen(
            ['sh', '-cxe', cmd], stderr=out, stdout=out, preexec_fn=preexec_fn)
        child_exit = p.wait()

        umount(mountpoint)

        if child_exit != 0:
            atexit.register(lambda: shutil.rmtree(new_child))
            raise BuildError(
                "build returned exit status {}".format(child_exit))

        final_child_dst = self._get_child_path(cmd)
        Container.by_node_path(final_child_dst).register_alias()
        try:
            os.rename(new_child, final_child_dst)
        except OSError as exc:
            if exc.errno == errno.ENOTEMPTY:
                info(
                    'This layer already exists builded and will not be replaced (layer: {})'.
                    format(layer_hash))
            else:
                raise
        self.add_layer(cmd)

    def add_or_build_layer(self, cmd, on_build=lambda: None, quiet=False):
        if not path.exists(self._get_child_path(cmd)):
            on_build()
            self.build_layer(cmd, quiet=quiet)
            used_cache = False
        else:
            used_cache = True
            self.add_layer(cmd)
        return used_cache

    def add_layer(self, cmd):
        self.layers.append(self._hash_cmd(cmd.encode()))
