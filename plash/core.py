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

BASE_DIR = '/var/lib/plash'
TMP_DIR = join(BASE_DIR, 'tmp')
BUILDS_DIR = join(BASE_DIR, 'builds')
LINKS_DIR = join(BASE_DIR, 'links')
LXC_URL_TEMPL = 'http://images.linuxcontainers.org/images/{}/{}/{}/{}/{}/rootfs.tar.xz' # FIXME: use https


class BuildError(Exception):
    pass

class CommandNotFound(Exception):
    pass

def umount(mountpoint):
    subprocess.check_call(['umount', '--lazy', '--recursive', mountpoint])

class BaseImageCreator:
    def __call__(self, image):

        self.arg = image
        image_id = self.get_id()
        image_dir = join(BUILDS_DIR, image_id)

        if not os.path.exists(image_dir):

            try:
                os.mkdir(BASE_DIR, 0o0700)
            except FileExistsError:
                pass
            try:
                os.mkdir(BUILDS_DIR)
            except FileExistsError:
                pass
            try:
                os.mkdir(TMP_DIR)
            except FileExistsError:
                pass
            try:
                os.mkdir(LINKS_DIR)
            except FileExistsError:
                pass

            tmp_image_dir = mkdtemp(dir=TMP_DIR) # must be on same fs than BASE_DIR for rename to work
            os.mkdir(join(tmp_image_dir, 'children'))
            os.mkdir(join(tmp_image_dir, 'payload'))
            rootfs = join(tmp_image_dir, 'payload')
            self.prepare_image(rootfs)

            # we want /etc/resolv to not by a symlink or not to not exist - that makes the later mount not work
            resolvconf = join(rootfs, 'etc/resolv.conf')
            try:
                os.unlink(resolvconf)
            except FileNotFoundError:
                pass
            with open(resolvconf, 'w') as f:
                f.seek(0)
                f.truncate()

            Container([image_id]).register_alias()
            try:
                os.rename(tmp_image_dir, image_dir)
            except OSError as exc:
                if exc.errno == errno.ENOTEMPTY:
                    info('another process already pulled that image')
                else:
                    raise
        
        return image_dir


class ImageNotFound(Exception):
    pass

class LXCImageCreator(BaseImageCreator):
    def get_id(self):
        return self.arg # XXX: dot dot attack and so son, escape or so

    def prepare_image(self, outdir):
        info('Preparing image')
        images = self._index_lxc_images()
        try:
            image_url = images[self.arg]
        except KeyError:
            raise ImageNotFound('No such image, available: {}'.format(
                ' '.join(sorted(images))))

        import tempfile
        _, download_file = tempfile.mkstemp(prefix=self.arg + '.', suffix='.tar.xz') # join(mkdtemp(), self.arg + '.tar.xz')
        run(['wget', '-q', '--show-progress', image_url, '-O', download_file])
        t = tarfile.open(download_file)
        t.extractall(outdir)

    def _index_lxc_images(self):
        content = urlopen('http://images.linuxcontainers.org/').read().decode() # FIXME: use https
        matches = re.findall('<tr><td>(.+?)</td><td>(.+?)</td><td>(.+?)</td><td>(.+?)</td><td>(.+?)</td><td>(.+?)</td><td>(.+?)</td><td>(.+?)</td></tr>', content)

        names = {}
        for distro, version, arch, variant, date, _, _, _ in matches:
            if not variant == 'default':
                continue

            if arch != 'amd64':  # only support this right now
                continue

            url = LXC_URL_TEMPL.format(distro, version, arch, variant, date)

            if version == 'current':
                names[distro] = url

            elif version[0].isalpha():
                # path parts with older upload_version also come later
                # (ignore possibel alphanumeric sort for dates on this right now)
                names[version] = url
            else:
                names['{}{}'.format(distro, version)] = url
        return names


class DirectoryImageCreator(BaseImageCreator):

    def get_id(self):
        return hashstr(self.arg.encode())

    def prepare_image(self, outdir):
        os.symlink(self.arg, outdir)


class SquashfsImageCreator(BaseImageCreator):
    def get_id(self):
        return self._hashfile(self.arg)

    def prepare_image(self, outdir):
        p = subprocess.Popen(['unsquashfs', '-d', outdir, self.arg])
        exit = p.wait()
        assert not exit

    def _hashfile(self, fname):
        hash_obj = hashlib.sha1()
        with open(fname, "rb") as f:
            for chunk in iter(lambda: f.read(4096), b""):
                hash_obj.update(chunk)
                return hash_obj.hexdigest()


class DockerImageCreator(BaseImageCreator):

    def _normalize_arg(self, image):
        if not '/' in image:
            image = 'library/' + image
        if not ':' in image:
            image += ':latest'
        return image

    def get_id(self):
        normalized_arg = self._normalize_arg(self.arg)
        return hashstr('dockerregistry {}'.format(normalized_arg).encode())

    def prepare_image(self, outdir):
        download_file = join(mkdtemp(), 'rootfs.tar.xz')
        p = subprocess.Popen(['docker', 'create', self._normalize_arg(self.arg)],
                         stdout=subprocess.PIPE)
        exit = p.wait()
        assert not exit
        result = p.stdout.read()
        container_id = result.decode().strip('\n')
        tar = mktemp()
        p = subprocess.Popen(['docker', 'export', '--output', tar, container_id])
        exit = p.wait()
        assert not exit
        t = tarfile.open(tar)
        t.extractall(outdir)


squashfs_image_creator = SquashfsImageCreator()
directory_image_creator = DirectoryImageCreator()
lxc_image_creator = LXCImageCreator()
docker_image_creator = DockerImageCreator()
def prepare_image(base_name):
    # return docker_image_creator(base_name)
    if base_name.startswith('/') or base_name.startswith('./'):
        path = abspath(base_name)
        if os.path.isdir(path):
            return directory_image_creator(path)
        else:
            return squashfs_image_creator(base_name)
    return lxc_image_creator(base_name)


def find_executable(programm, root=None):

    if root:
        preexec_fn = lambda: os.chroot(root)
    else:
        preexec_fn = None

    p = subprocess.Popen(['sh', '-c', 'command -v ' + shlex.quote(programm)], stdout=subprocess.PIPE, preexec_fn=preexec_fn)
    p.wait()
    found_source_binary =  p.stdout.read().decode().strip("\n")
    if not found_source_binary:
        raise ValueError("No such program found: {}".format(programm))
    return found_source_binary

class ContainerDoesNotExist(Exception):
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
            abs_last_layer_path = path.abspath(path.join(LINKS_DIR, last_layer_path))
            if not os.path.exists(abs_last_layer_path):
                error = True
        if error:
            raise ContainerDoesNotExist('no such container: {}'.format(alias))
        layers = last_layer_path[len('../builds/'):].split('/children/')
        return cls(layers)

    @classmethod
    def by_node_path(cls, node_path):
        layers = node_path[len(BUILDS_DIR)+1:].split('/children/') # brittle
        return cls(layers)

    @property
    def alias(self):
        return self.layers[-1]

    def register_alias(self):
        alias = self.alias
        try:
            os.symlink(self.get_node_path(relative=True), join(LINKS_DIR, alias))
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
        return '../builds/'+'/children/'.join(self.layers) # very brittle

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
        # run(['mount', '--bind', '/dev/pts', join(mountpoint, 'dev', 'pts')]) # apt-get needs that
        # run(['mount', '--bind', '/dev/shm', join(mountpoint, 'dev', 'shm')])
        run(['mount', '-t', 'tmpfs', 'tmpfs', join(mountpoint, 'tmp')]) 

        run(['mount', '--bind', '/etc/resolv.conf', join(mountpoint, 'etc/resolv.conf')])

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
            'mount',
            '-t',
            'overlay',
            'overlay',
            '-o',
            'upperdir={write_dir},lowerdir={dirs},workdir={workdir}'.format(
                write_dir=write_dir,
                workdir=workdir,
                dirs=':'.join(join(p, 'payload') for p in reversed(symlinked_layer_paths))),
            mountpoint]
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
            os.close(0) # close stdin for more reproducible builds - if that does not work well, there is another way

        if quiet:
            out = subprocess.DEVNULL
        else:
            out = 2 # stderr
        p = subprocess.Popen(['sh', '-cxe', cmd], stderr=out, stdout=out, preexec_fn=preexec_fn)
        child_exit = p.wait()

        umount(mountpoint)

        if child_exit != 0:
            atexit.register(lambda: shutil.rmtree(new_child))
            raise BuildError("build returned exit status {}".format(child_exit))

        final_child_dst = self._get_child_path(cmd)
        Container.by_node_path(final_child_dst).register_alias()
        try:
            os.rename(new_child, final_child_dst)
        except OSError as exc:
            if exc.errno == errno.ENOTEMPTY:
                info('This layer already exists builded and will not be replaced (layer: {})'.format(layer_hash))
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

    def create_runnable(self, source_binary, runnable):
        # SECURITY: fix permissions
        mountpoint = mkdtemp(dir=TMP_DIR)
        self.mount_rootfs(mountpoint=mountpoint)
        os.chmod(mountpoint, 0o755) # that permission the root directory '/' needs

        if not '/' in source_binary:
            source_binary = find_executable(source_binary, root=mountpoint)

        # if runnable != '/entrypoint': # if it is we dont't need to create this "link" (this is a little smartasssish)
        #     with open(join(mountpoint, 'entrypoint'), 'w') as f:
        #         f.write('#!/bin/sh\n') # put in one call
        #         f.write('exec {} $@'.format(shlex.quote(source_binary)))
        #     os.chmod(join(mountpoint, 'entrypoint'), 0o755)
        etc_runp = join(mountpoint, 'etc/runp')
        if not os.path.exists(etc_runp):
            os.mkdir(etc_runp)
        os.symlink(source_binary, join(etc_runp, 'exec'))

        # create that file so we can overmount it
        with open(join(mountpoint, 'etc', 'resolv.conf'), 'a') as _:
            pass

        # os.symlink(executable, join(mountpoint, 'entrypoint'))
        print('Squashing... ', end='', flush=True, file=sys.stderr)
        subprocess.check_call(['mksquashfs', mountpoint, runnable + '.squashfs', '-comp', 'xz', '-noappend'], stdout=subprocess.DEVNULL)

        os.symlink('/home/resu/plash/runp', runnable) # fixme: take if from /usr/bin/runp 
        print('OK', file=sys.stderr)
    

    def run(self, cmd):

        # SECURITY: fix permissions
        mountpoint_wrapper = mkdtemp(dir='/var/tmp') # don't use /tmp because its mounted on the container, that would cause weird mount recursion
        mountpoint = join(mountpoint_wrapper, 'env.dir')
        os.mkdir(mountpoint)
        os.symlink('/usr/local/bin/runp', join(mountpoint_wrapper, 'env'))
        self.mount_rootfs(mountpoint=mountpoint)

        # just for  a nicer error message for the user
        if not os.fork():
            os.chroot(mountpoint)
            found = shutil.which(cmd[0])
            sys.exit(int(not bool(found)))
        _, exit = os.wait()
        if exit // 256:
            raise CommandNotFound('command not found: {}'.format(repr(cmd[0])))

        etc_runp = join(mountpoint, 'etc/runp')
        if not os.path.exists(etc_runp):
            os.mkdir(etc_runp)
        os.symlink('/usr/bin/env', join(etc_runp, 'exec'))

        os.chmod(mountpoint_wrapper, 0o755)
        os.chmod(mountpoint, 0o755)
        deescalate_sudo()
        cmd = [join(mountpoint_wrapper, 'env')] + cmd
        os.execvpe(cmd[0], cmd, os.environ)
