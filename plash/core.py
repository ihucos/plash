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

from .utils import hashstr, run, deescalate_sudo

BASE_DIR = '/var/lib/plash'
TMP_DIR = join(BASE_DIR, 'tmp')
BUILDS_DIR = join(BASE_DIR, 'builds')
LXC_URL_TEMPL = 'http://images.linuxcontainers.org/images/{}/{}/{}/{}/{}/rootfs.tar.xz' # FIXME: use https


class BuildError(Exception):
    pass


# HACK HACK NOT HERE
# this should be in utils and explicitely registered
# for usability its really nice, we can give the user a message when C-c is hit
# and it als quits more idmediately
import signal
def signal_handler(signal, frame):
    print('Interrupted by user')
    sys.exit(0)
signal.signal(signal.SIGINT, signal_handler)

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

            tmp_image_dir = mkdtemp(dir=TMP_DIR) # must be on same fs than BASE_DIR for rename to work
            os.mkdir(join(tmp_image_dir, 'children'))
            os.mkdir(join(tmp_image_dir, 'payload'))
            self.prepare_image(join(tmp_image_dir, 'payload'))

            try:
                os.rename(tmp_image_dir, image_dir)
            except OSError as exc:
                if exc.errno == errno.ENOTEMPTY:
                    print('*** plash: another process already pulled that image', file=sys.stderr)
                else:
                    raise
        
        return image_dir


class LXCImageCreator(BaseImageCreator):
    def get_id(self):
        return self.arg # XXX: dot dot attack and so son, escape or so

    def prepare_image(self, outdir):
        print('Fetching image index...')
        images = self._index_lxc_images()
        try:
            image_url = images[self.arg]
        except KeyError:
            raise ValueError('No such image, available: {}'.format(
                ' '.join(sorted(images))))

        import tempfile
        _, download_file = tempfile.mkstemp(prefix=self.arg + '.', suffix='.tar.xz') # join(mkdtemp(), self.arg + '.tar.xz')
        run(['wget', '-q', '--show-progress', image_url, '-O', download_file])
        print('Unpacking...')
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
def bootstrap_base_rootfs(base_name):
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

class Container:

    def __init__(self, container_id=''):
        self._layer_ids = container_id.split(':')

    # def _get_last_layer_salt_file(self):
    #     return join(self.get_layer_paths()[-1], 'salt')

    def ensure_base(self):
        assert self._layer_ids
        bootstrap_base_rootfs(self._layer_ids[0])

    def _get_child_path(self, cmd):
        layer_hash = self._hash_cmd(cmd.encode())
        last_layer = self.get_layer_paths()[-1]
        return join(last_layer, 'children', layer_hash)

    def _hash_cmd(self, cmd):
        # self.get_layer_paths()[-1]
        return hashstr(cmd)[:12]

    def get_layer_paths(self):
        lp = [join(BUILDS_DIR, self._layer_ids[0])]
        for ci in self._layer_ids[1:]:
            lp.append(lp[-1] + '/children/' + ci)
        return lp

    def is_builded(self):
        last_layer = self.get_layer_paths()[-1]
        return os.path.exists(last_layer)

    def log_access(self):
        for path in reversed(self.get_layer_paths()):
            os.utime(path, None)

    def _prepare_chroot(self, mountpoint):
        run(['mount', '-t', 'proc', 'proc', join(mountpoint, 'proc')])
        run(['mount', '--bind', '/home', join(mountpoint, 'home')])
        run(['mount', '--bind', '/sys', join(mountpoint, 'sys')])
        run(['mount', '--bind', '/dev', join(mountpoint, 'dev')])
        # run(['mount', '--bind', '/dev/pts', join(mountpoint, 'dev', 'pts')]) # apt-get needs that
        # run(['mount', '--bind', '/dev/shm', join(mountpoint, 'dev', 'shm')])
        run(['mount', '-t', 'tmpfs', 'tmpfs', join(mountpoint, 'tmp')]) 

        # run(['mount', '--bind', '/var/lib/runc/mydata', join(mountpoint, '/mnt')])

        # kind of hacky, we create an empty etc/resolv.conf so we can mount over it if it does not exists
        resolv_file = join(mountpoint, 'etc/resolv.conf')
        if not os.path.exists(resolv_file):
            with open(resolv_file, 'w') as _:
                pass
        run(['mount', '--bind', '/etc/resolv.conf', resolv_file])

    def mount_rootfs(self, *, mountpoint, write_dir=None, workdir=None):
        if write_dir == None:
            write_dir = mkdtemp(dir=TMP_DIR)
        if workdir == None:
            workdir = mkdtemp(dir=TMP_DIR)
        cmd = [
            'mount',
            '-t',
            'overlay',
            'overlay',
            '-o',
            'upperdir={write_dir},lowerdir={dirs},workdir={workdir}'.format(
                write_dir=write_dir,
                workdir=workdir,
                dirs=':'.join(join(p, 'payload') for p in reversed(self.get_layer_paths()))),
            mountpoint]
        run(cmd)
        return mountpoint

    def build_layer(self, cmd):
        print('*** plash: building layer', file=sys.stderr)
        new_child = mkdtemp(dir=TMP_DIR)
        mountpoint = mkdtemp(dir=TMP_DIR)
        new_layer = join(new_child, 'payload')
        os.mkdir(new_layer)
        os.mkdir(join(new_child, 'children'))

        self.mount_rootfs(mountpoint=mountpoint, write_dir=new_layer)

        self._prepare_chroot(mountpoint)

        if not os.fork():
            os.chroot(mountpoint)
            os.chdir("/")

            # don't allow build processes to read from stdin, since we want as "deterministic as possible" builds
            fd = os.open("/dev/null", os.O_WRONLY)
            os.dup2(fd, 0);
            os.close(fd);
            os.dup2(2, 1);

            shell = 'sh'
            os.execvpe(shell, [shell, '-cxe', cmd], os.environ) # maybe isolate envs better?
        child_pid, child_exit = os.wait()

        umount(mountpoint)

        if child_exit != 0:
            print("*** plash: build failed with exit status: {}".format(child_exit // 256), file=sys.stderr)
            shutil.rmtree(new_child)
            sys.exit(1)

        final_child_dst = self._get_child_path(cmd)
        try:
            os.rename(new_child, final_child_dst)
        except OSError as exc:
            if exc.errno == errno.ENOTEMPTY:
                print('*** plash: this layer already exists builded and will not be replaced (layer: {})'.format(layer_hash), file=sys.stderr)
            else:
                raise

    def add_or_build_layer(self, cmd):
        if not path.exists(self._get_child_path(cmd)):
            self.build_layer(cmd)
        else:
            pass
            # print('*** plash: cached layer', file=sys.stderr)
        self.add_layer(cmd)
    
    def add_layer(self, cmd):
        self._layer_ids.append(self._hash_cmd(cmd.encode()))
    

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
        print('Squashing... ', end='', flush=True)
        subprocess.check_call(['mksquashfs', mountpoint, runnable + '.squashfs', '-comp', 'xz', '-noappend'], stdout=subprocess.DEVNULL)

        os.symlink('/home/resu/plash/runp', runnable) # fixme: take if from /usr/bin/runp 
        print('OK')
    
    def docker_export(self):
        mountpoint = mkdtemp(dir=TMP_DIR)
        self.mount_rootfs(mountpoint=mountpoint)
        os.chmod(mountpoint, 0o755) # that permission the root directory '/' needs
        subprocess.check_call(['tar', '-cz', 'mountpoint'])
    
    def run(self, cmd):

        # SECURITY: fix permissions
        mountpoint_wrapper = mkdtemp(dir='/var/tmp') # don't use /tmp because its mounted on the container, that would cause weird mount recursion
        mountpoint = join(mountpoint_wrapper, 'env.dir')
        os.mkdir(mountpoint, 0o755)
        os.symlink('/usr/local/bin/runp', join(mountpoint_wrapper, 'env'))
        self.mount_rootfs(mountpoint=mountpoint)

        etc_runp = join(mountpoint, 'etc/runp')
        if not os.path.exists(etc_runp):
            os.mkdir(etc_runp)
        os.symlink('/usr/bin/env', join(etc_runp, 'exec'))

        cmd = [join(mountpoint_wrapper, 'env')] + cmd
        os.execvpe(cmd[0], cmd, os.environ)

    def __repr__(self):
        return ':'.join(self._layer_ids)

# def execute(
#         base_name,
#         layer_commands,
#         command,
#         *,
#         # quiet_flag=False,
#         # verbose_flag=False,
#         # rebuild_flag=False,
#         # extra_mounts=[],
#         # build_only=False,
#         # skip_if_exists=True,
#         export_as=False,
#         # docker_image=False,
#         # extra_envs={}
#         **kw):


#     c = Container(base_name)
#     c.ensure_base()
#     for cmd in layer_commands:
#         c.add_or_build_layer(cmd)
#     if export_as:
#         c.create_runnable(export_as)
#     else:
#         if not command:
#             print('*** plash: build is ready')
#         else:
#             c.run(command)
