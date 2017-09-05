import errno
import os.path
import shutil
import subprocess
from os.path import join
from tempfile import mkdtemp

from plash.utils import hashstr, run

TMP_DIR = '/tmp'
LXC_URL_TEMPL = 'http://images.linuxcontainers.org/images/{}/{}/{}/{}/{}/rootfs.tar.xz' # FIXME: use https

def reporthook(counter, buffer_size, size):
      expected_ticks = int(size / buffer_size)
      dot_every_ticks = int(expected_ticks / 40)
      if counter % dot_every_ticks == 0:
            dot_count = counter / dot_every_ticks

            if dot_count % 4 == 0:
                  countdown = 10 - int(round((counter * buffer_size / size) * 10))
                  if countdown != 10:
                        print(' ', end='', flush=True)
                  if countdown == 0:
                        print('ready', flush=True)
                  else:
                        print(countdown, end='', flush=True)
            else:
                  print('.', end='', flush=True)


def hashfile(fname):
    hash_obj = hashlib.sha1()
    with open(fname, "rb") as f:
        for chunk in iter(lambda: f.read(4096), b""):
            hash_obj.update(chunk)
            return hash_obj.hexdigest()

def index_lxc_images():
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

class BaseImageCreator:
    def __call__(self, image):

        self.arg = image
        image_id = self.get_id()
        image_dir = join(BUILDS_DIR, image_id)

        if not os.path.exists(image_dir):

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
        print('getting images index')
        images = index_lxc_images()
        try:
            image_url = images[image]
        except KeyError:
            raise ValueError('No such image, available: {}'.format(
                ' '.join(sorted(images))))

        download_file = join(tmp_image_dir, 'download')
        print('Downloading image: ', end='', flush=True)
        urlretrieve(image_url, download_file, reporthook=reporthook)
        t = tarfile.open(download_file)
        t.extractall(outdir)

# class DockerImageCreator(BaseImageCreator):
#     pass


class DirectoryImageCreator(BaseImageCreator):

    def get_id(self):
        return hashstr(self.arg.encode())

    def prepare_image(self, outdir):
        os.symlink(self.arg, outdir)


class SquashfsImageCreator(BaseImageCreator):
    def get_id(self):
        return hashfile(self.arg)

    def prepare_image(self, outdir):
        p = subprocess.Popen(['unsquashfs', '-d', outdir, self.arg])
        exit = p.wait()
        assert not exit


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

    def __init__(self, container_id):
        self._layer_ids = container_id.split(':')

    def hash_cmd(self, cmd):
        return hashstr(cmd)[:12]

    def get_layer_paths(self):
        lp = ["/var/lib/plash/builds/{}".format(self._layer_ids[0])]
        for ci in self._layer_ids[1:]:
            lp.append(lp[-1] + '/children/' + ci)
        # print(lp)
        return lp

    def invalidate(self):
        pass

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

    def prepare_chroot(self, mountpoint):
        run(['mount', '-t', 'proc', 'proc', join(mountpoint, 'proc')])
        run(['mount', '--bind', '/sys', join(mountpoint, 'sys')])
        run(['mount', '--bind', '/dev', join(mountpoint, 'dev')])
        run(['mount', '--bind', '/dev/pts', join(mountpoint, 'dev', 'pts')])
        run(['mount', '--bind', '/dev/shm', join(mountpoint, 'dev', 'shm')])
        run(['mount', '--bind', '/tmp', join(mountpoint, 'tmp')])

    def build_layer(self, cmd):
        print('*** plash: building layer')
        new_child = mkdtemp(dir=TMP_DIR)
        new_layer = join(new_child, 'payload')
        os.mkdir(new_layer)
        os.mkdir(join(new_child, 'children'))

        self.mount_rootfs(mountpoint=new_layer)

        self.prepare_chroot(new_layer)

        if not os.fork():
            os.chroot(new_layer)
            # from time import sleep
            # sleep(5)
            shell = 'sh'
            os.execvpe(shell, [shell, '-ce', cmd], os.environ) # maybe isolate envs better?
        child_pid, child_exit = os.wait()

        run(["umount", "--recursive", new_layer])

        if child_exit != 0:
            print("*** plash: build failed with exit status: {}".format(child_exit))
            shutil.rmtree(new_child)
            sys.exit(1)


        last_layer = self.get_layer_paths()[-1]
        layer_hash = self.hash_cmd(cmd.encode())
        final_child_dst = join(last_layer, 'children', layer_hash)
        try:
            os.rename(new_child, final_child_dst)
        except OSError as exc:
            if exc.errno == errno.ENOTEMPTY:
                print('*** plash: this layer already exists builded and will not be replaced (layer: {})'.format(layer_hash))
            else:
                raise

        return self._layer_ids.append(layer_hash)

    def is_builded(self, cmd):
        layer_hash = self.hash_cmd(cmd.encode())
        last_layer = self.get_layer_paths()[-1]
        final_child_dst = join(last_layer, 'children', layer_hash)
        return os.path.exists(final_child_dst)


    def add_layer(self, cmd):
        if not self.is_builded(cmd):
            return self.build_layer(cmd)
        else:
            print('*** plash: cached layer')

    def create_runnable(self, file, executable, *, verbose_flag=False):
        mountpoint = mkdtemp(dir=TMP_DIR)
        self.mount_rootfs(mountpoint=mountpoint)
        os.chmod(mountpoint, 0o755) # that permission the root directory '/' needs

        # with open(join(mountpoint, 'entrypoint'), 'w') as f:
            # f.write('#!/bin/sh\n') # put in one call
            # f.write('exec {} $@'.format(' '.join(shlex.quote(i) for i in command)))
        # os.chmod(join(mountpoint, 'entrypoint'), 0o755)
        os.symlink(executable, join(mountpoint, 'entrypoint'))
        run(['mksquashfs', mountpoint, file, '-Xcompression-level', '1'])
    
    def run(self, cmd):
        assert isinstance(cmd, list)
        cached_file = join(TMP_DIR, hashstr(' '.join(self._layer_ids).encode())) # SECURITY: check that file owner is root -- but then timing attack possible!
        if not os.path.exists(cached_file):
            self.create_runnable(cached_file, '/usr/bin/env')
        cmd = ['/home/resu/plash/runp', cached_file] + cmd
        os.execvpe(cmd[0], cmd, os.environ)

def execute(
        base_name,
        layer_commands,
        command,
        *,
        # quiet_flag=False,
        # verbose_flag=False,
        # rebuild_flag=False,
        # extra_mounts=[],
        # build_only=False,
        # skip_if_exists=True,
        export_as=False,
        # docker_image=False,
        # su=False,
        # extra_envs={}
        **kw):


    c = Container(base_name)
    for cmd in layer_commands:
        c.add_layer(cmd)
    if export_as:
        if not command:
            print("if export_as you must supply a command")
            sys.exit(1)
        if len(command) == 1:
            print("if export_as the command must be a binary")
            sys.exit(1)
        c.create_runnable(export_as, export_binary)
    else:
        c.run(cmd)
