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

from .utils import hashstr, run

TMP_DIR = '/tmp'
LXC_URL_TEMPL = 'http://images.linuxcontainers.org/images/{}/{}/{}/{}/{}/rootfs.tar.xz' # FIXME: use https


class BuildError(Exception):
    pass


class BaseImageCreator:
    def __call__(self, image):

        self.arg = image
        image_id = self.get_id()
        image_dir = join('/var/lib/plash', image_id)

        if not os.path.exists(image_dir):

            try:
                os.mkdir('/var/lib/plash', 0o0700)
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
                    print('*** plash: another process already pulled that image')
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
        lp = ["/var/lib/plash/{}".format(self._layer_ids[0])]
        for ci in self._layer_ids[1:]:
            lp.append(lp[-1] + '/children/' + ci)
        return lp

    def log_access(self):
        for path in reversed(self.get_layer_paths()):
            os.utime(path, None)

    def _prepare_chroot(self, mountpoint):
        run(['mount', '-t', 'proc', 'proc', join(mountpoint, 'proc')])
        run(['mount', '--bind', '/home', join(mountpoint, 'home')])
        run(['mount', '--bind', '/sys', join(mountpoint, 'sys')])
        run(['mount', '--bind', '/dev', join(mountpoint, 'dev')])
        run(['mount', '--bind', '/dev/pts', join(mountpoint, 'dev', 'pts')]) # apt-get needs that
        # run(['mount', '--bind', '/dev/shm', join(mountpoint, 'dev', 'shm')])
        run(['mount', '-t', 'tmpfs', 'tmpfs', join(mountpoint, 'tmp')]) 

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
                dirs=':'.join(join(p, 'payload') for p in self.get_layer_paths())),
            mountpoint]
        run(cmd)
        return mountpoint

    def build_layer(self, cmd):
        print('*** plash: building layer')
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

            shell = 'sh'
            os.execvpe(shell, [shell, '-cxe', cmd], os.environ) # maybe isolate envs better?
        child_pid, child_exit = os.wait()

        run(["umount", "--recursive", mountpoint])

        if child_exit != 0:
            print("*** plash: build failed with exit status: {}".format(child_exit // 256))
            shutil.rmtree(new_child)
            sys.exit(1)

        final_child_dst = self._get_child_path(cmd)
        try:
            os.rename(new_child, final_child_dst)
        except OSError as exc:
            if exc.errno == errno.ENOTEMPTY:
                print('*** plash: this layer already exists builded and will not be replaced (layer: {})'.format(layer_hash))
            else:
                raise

    def add_or_build_layer(self, cmd):
        if not path.exists(self._get_child_path(cmd)):
            self.build_layer(cmd)
        else:
            pass
            # print('*** plash: cached layer')
        self.add_layer(cmd)
    
    def add_layer(self, cmd):
        self._layer_ids.append(self._hash_cmd(cmd.encode()))

    def create_runnable(self, source_binary, runnable):
        # SECURITY: fix permissions
        mountpoint = mkdtemp(dir=TMP_DIR)
        self.mount_rootfs(mountpoint=mountpoint)
        os.chmod(mountpoint, 0o755) # that permission the root directory '/' needs

        if not '/' in source_binary:
            p = subprocess.Popen(['sh', '-c', 'command -v ' + shlex.quote(source_binary)], stdout=subprocess.PIPE, preexec_fn=lambda: os.chroot(mountpoint))
            p.wait()
            found_source_binary =  p.stdout.read().decode().strip("\n")
            if not found_source_binary:
                raise ValueError("No such program found in container: {}".format(source_binary))
            source_binary = found_source_binary

        # if runnable != '/entrypoint': # if it is we dont't need to create this "link" (this is a little smartasssish)
        #     with open(join(mountpoint, 'entrypoint'), 'w') as f:
        #         f.write('#!/bin/sh\n') # put in one call
        #         f.write('exec {} $@'.format(shlex.quote(source_binary)))
        #     os.chmod(join(mountpoint, 'entrypoint'), 0o755)
        os.symlink(source_binary, join(mountpoint, 'etc/runp_exec'))

        # create that file so we can overmount it
        with open(join(mountpoint, 'etc', 'resolv.conf'), 'a') as _:
            pass

        # os.symlink(executable, join(mountpoint, 'entrypoint'))
        print('Squashing... ', end='', flush=True)
        subprocess.check_call(['mksquashfs', mountpoint, runnable + '.squashfs', '-Xcompression-level', '1', '-noappend'], stdout=subprocess.DEVNULL)

        os.symlink('/home/resu/plash/runp', runnable) # fixme: take if from /usr/bin/runp 
        print('OK')
    
    def run(self, cmd):

        # SECURITY: fix permissions
        mountpoint = mkdtemp(dir='/var/tmp') # don't use /tmp because its mounted on the container, that would cause weird mount recursion
        os.chmod(mountpoint, 0o755) # that permission the root directory '/' needs
        self.mount_rootfs(mountpoint=mountpoint)

        # mounting that as we currently do in runp.go is really fucked up, on ubuntu because of symlink umounting it again does not work
        with open(join(mountpoint, 'etc', 'resolv.conf'), 'w') as f:
            f.write(open('/etc/resolv.conf').read())

        # same as what is in runp.go, keep both in sync!
        run(["/bin/mount", "-t", "proc", "proc", mountpoint + "/proc"])
        # run(["/bin/mount", "--bind", "-o", "ro", "/etc/resolv.conf", mountpoint + "/etc/resolv.conf"])
        for mount in ["/sys", "/dev", "/tmp", "/home", "/run"]:
            run(["/bin/mount", "--bind", mount, mountpoint + mount])
        if not os.fork():
            old_pwd = os.getcwd()
            os.chroot(mountpoint)
            os.chdir('/')
            try:
                os.chdir(old_pwd)
            except FileNotFoundError:
                pass # we are fine staying at /
            os.execvpe(cmd[0], cmd, os.environ)
        _, child_exit = os.wait()

        run(["/bin/umount", "--recursive", mountpoint])
        os.rmdir(mountpoint)
        print("*** plash: program exit status {}".format(child_exit // 256)) # that // 256 is one of these things i don't fully understand


        # # XXX: run with chroot and exec instead!!
        # assert isinstance(cmd, list)
        # cached_file = join(TMP_DIR, 'plash-' + hashstr(' '.join(self._layer_ids).encode())) # SECURITY: check that file owner is root -- but then timing attack possible!
        # if not os.path.exists(cached_file):
        #     self.create_runnable(cached_file, ['/usr/bin/env'])
        # cmd = [cached_file] + cmd
        # os.execvpe(cmd[0], cmd, os.environ)
    
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
