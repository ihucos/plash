
'''
file to mount in host: https://github.com/proot-me/PRoot/blob/master/doc/proot/manual.txt
* /etc/host.conf
* /etc/hosts
* /etc/nsswitch.conf
* /etc/resolv.conf
* /dev/
* /sys/
* /proc/
* /tmp/
* /run/shm

root #mount -t proc none /foo/proc
root #mount -o bind /dev /foo/dev
root #mount -o bind /usr/portage /foo/usr/portage
root #mount -o bind /usr/src/linux /foo/usr/src/linux
root #mount -o bind /lib/modules /foo/lib/modules
root #mount -o bind /sys /foo/sys
root #cp /etc/resolv.conf /foo/etc/resolv.conf
'''


import errno
import hashlib
import lzma  # XXX: we need that but dont use directly, apt install python-lzma
import os
import re
import shlex
import shutil
import sqlite3
import subprocess
import tarfile
import time
from os.path import abspath, join
from shutil import rmtree
from sys import argv
from tempfile import mkdtemp
from urllib.request import urlopen, urlretrieve

from .utils import NonZeroExitStatus, friendly_exception, hashstr, run

BASE_DIR = os.environ.get('PLASH_DATA', '/var/lib/plash')
TMP_DIR = join(BASE_DIR, 'tmp')
MNT_DIR = join(BASE_DIR, 'mnt')
BUILDS_DIR = join(BASE_DIR, 'builds')
LXC_URL_TEMPL = 'http://images.linuxcontainers.org/images/{}/{}/{}/{}/{}/rootfs.tar.xz' # FIXME: use https

def hashfile(fname):
    hash_obj = hashlib.sha1()
    with open(fname, "rb") as f:
        for chunk in iter(lambda: f.read(4096), b""):
            hash_obj.update(chunk)
            return hash_obj.hexdigest()

class BuildError(Exception):
    pass

def pidsuffix():
    return '.{}'.format(os.getpid())

def prepare_data_dir(data_dir):
    for dir in [BASE_DIR, TMP_DIR, MNT_DIR, BUILDS_DIR]:
        try:
            os.mkdir(dir)
        except FileExistsError:
            pass


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

def staple_layer(layers, layer_cmd, rebuild=False):
    last_layer = layers[-1]
    layer_name = hashstr(' '.join(layers + [layer_cmd]).encode())
    final_child_dst = join(last_layer, 'children', layer_name)

    if not os.path.exists(final_child_dst) or rebuild:
        print('*** plash: building layer')
        new_child = mkdtemp(dir=TMP_DIR)
        new_layer = join(new_child, 'payload')
        os.mkdir(new_layer)
        os.mkdir(join(new_child, 'children'))

        mountpoint = mount_layers(layers=[join(i, 'payload') for i in layers], write_dir=new_layer)

        p = subprocess.Popen(
            [argv[0],
             'chroot',
             '--cow', new_layer,
             mountpoint,
             '--', 'sh', '-ce', layer_cmd], stdout=2, stderr=2)
        exit = p.wait()
        umount(mountpoint)
        assert exit == 0, 'Building returned non zero exit status'
        if exit != 0:
            print('non zero exit status code when building')
            shutil.rmtree(build_at)
            assert False
        else:
            try:

                # if rebuilding get rid of current build if there is one
                # this could be done with one call with renameat2
                if rebuild:
                    try:
                        os.rename(final_child_dst, mkdtemp(dir=TMP_DIR))
                    except FileNotFoundError as exc:
                        pass
                    except OSError as exc:
                        if exc.errno != errno.ENOTEMPTY:
                            raise


                os.rename(new_child, final_child_dst)
            except OSError as exc:
                if exc.errno == errno.ENOTEMPTY:
                    print('*** plash: this layer already exists builded and will not be replaced (layer: {})'.format(layer_name))
                else:
                    raise

    else:
        print('*** plash: cached layer'.format(layer_name))

    return layers + [final_child_dst]


def umount(mountpoint):
    run(['umount', '--recursive', mountpoint])
    
def mount_layers(layers, write_dir):
    mountpoint = mkdtemp(dir=MNT_DIR, suffix=pidsuffix()) # save pid so we can unmout it when that pid dies
    workdir = mkdtemp(dir=MNT_DIR)
    layers=list(layers)
    cmd = [
        'mount',
        '-t',
        'overlay',
        'overlay',
        '-o',
        'upperdir={write_dir},lowerdir={dirs},workdir={workdir}'.format(
            write_dir=write_dir,
            workdir=workdir,
            dirs=':'.join(i for i in layers)),
        mountpoint]
    run(cmd)
    return mountpoint


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
            raise ValueError('No such image, available: {}'.format(' '.join(sorted(images))))

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

    def prepare(self, outdir):
        p = subprocess.Popen(['unsquashfs', '-d', outdir, self.arg])
        exit = p.wait()
        assert not exit

squashfs_image_creator = SquashfsImageCreator()
directory_image_creator = DirectoryImageCreator()
lxc_image_creator = LXCImageCreator()
def prepare_base(base_name):
    if base_name.startswith('/') or base_name.startswith('./'):
        path = abspath(base_name)
        if os.path.isdir(path):
            return directory_image_creator(path)
        else:
            return squashfs_image_creator(base_name)
    return lxc_image_creator(base_name)


# def pull_base(image):

#     # normalize image name

#     if not '/' in image:
#         image = 'library/' + image
#     if not ':' in image:
#         image += ':latest'

#     image_dir = join(BUILDS_DIR, join(hashstr('dockerregistry {}'.format(image).encode())))
#     if not os.path.exists(image_dir):
#         print('*** plash: preparing {}'.format(image))
#         download_file = join(mkdtemp(), 'rootfs.tar.xz')
#         tmp_image_dir = mkdtemp(dir=TMP_DIR) # must be on same fs than BASE_DIR for rename to work
#         os.mkdir(join(tmp_image_dir, 'children'))
#         os.mkdir(join(tmp_image_dir, 'payload'))

#         # also doable with skopeo and oci-image-tool
#         # run(['wget', 'https://us.images.linuxcontainers.org/images/ubuntu/artful/amd64/default/20170729_03:49/rootfs.tar.xz', '-O', download_file])
#         # run(['tar', 'xf', download_file, '--exclude=./dev', '-C', join(tmp_image_dir, 'payload')])

#         # subprocess.check_output(['docker', 'create', image])
#         run(['bash', '-c', 'docker export $(docker create '+image+') | tar -C '+join(tmp_image_dir, 'payload')+' --exclude=/dev --exclude=/proc --exclude=/sys -xf -']) # command injection!! # excluding is not working!

#         try:
#             os.rename(tmp_image_dir, image_dir)
#         except OSError as exc:
#             if exc.errno == errno.ENOTEMPTY:
#                 print('*** plash: another process already pulled that image')

#     return image_dir

def build(image, layers, *, quiet_flag=False, verbose_flag=False, rebuild_flag=False):
    base = [image]
    for layer in layers:
        base = staple_layer(base, layer, rebuild=rebuild_flag)
    return base


def execute(
        base_name,
        layer_commands,
        command,
        *,
        quiet_flag=False,
        verbose_flag=False,
        rebuild_flag=False,
        extra_mounts=[],
        build_only=False,
        skip_if_exists=True,
        export_as=False,
        extra_envs={}):

    if export_as and not command:
        raise ValueError('if export_as is specified, a command also must be')

    # assert 0, layer_commands
    prepare_data_dir(BASE_DIR)
    base_dir = prepare_base(base_name)
    layers = build(base_dir, layer_commands, rebuild_flag=rebuild_flag)

    # update that we used this layers
    for layer in layers:
        os.utime(join(layer), None)

    if build_only:
        print('*** plash: Build is ready at: {}'.format(layers[-1]))
    else:
        mountpoint = mount_layers([join(i, 'payload') for i in layers], mkdtemp(dir=TMP_DIR))
        os.chmod(mountpoint, 0o755) # that permission the root directory '/' needs
        # subprocess.Popen(['chmod', '755', mountpoint])
        last_layer = layers[-1]
        # os.utime(join(last_layer, 'lastused'), times=None) # update the timestamp on this

        if export_as:
            with open(join(mountpoint, 'entrypoint'), 'w') as f:
                f.write('#!/bin/sh\n') # put in one call
                f.write('exec {} $@'.format(' '.join(shlex.quote(i) for i in command)))
            os.chmod(join(mountpoint, 'entrypoint'), 0o755)
            p = subprocess.Popen(['mksquashfs', mountpoint, export_as])
            assert not p.wait(), 'bad exit code'
        else:
            execcmd = [argv[0], 'chroot', '--mount-home', '--cwd', os.getcwd(), mountpoint, '--'] + command
            os.execvpe(execcmd[0], execcmd, extra_envs)
            # pwd = os.getcwd()
            # prepare_rootfs(mountpoint)
            # os.chroot(mountpoint)

            # os.chdir(pwd)
            # # try:
            # # except OSError, exc:
            # #     if exc.errno == 2: # no such file or directory
            # #         os.chdir('/')

            # uid = os.environ.get('SUDO_UID')
            # if uid:
            #     os.setgid(int(os.environ['SUDO_GID']))
            #     os.setuid(int(uid))

            # with friendly_exception([FileNotFoundError]):
            #     os.execvpe(command[0], command, extra_envs)


# this as a separate script!

# cleanup_mounts()
# import sys; sys.exit(0)



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

if __name__ == '__main__':
    # print(staple_layer(['/tmp/data/layers/ubuntu'], 'touch a'))
    # print(staple_layer(staple_layer(['/tmp/data/layers/ubuntu'], 'touch a'), 'touch b'))
    # print(build('/tmp/data/layers/ubuntu', ['touch a', 'touch b', 'rm a']))
    execute('ubuntu:16.04', ['touch a', 'touch b', 'rm /a', 'apt-get update && apt-get install cowsay'], ['/usr/games/cowsay', 'hi'], rebuild_flag=False, verbose_flag=True)
    # call('ubuntu:16.04', ['touch ac', 'touch b', 'touch c'], ['/usr/games/cowsay', 'hi'], rebuild_flag=False, verbose_flag=True)
    # import cProfile
    # cProfile.run("call('ubuntu:16.04', ['touch a', 'touch b', 'rm /a', 'apt-get update && apt-get install cowsay'], ['/usr/games/cowsay', 'hi'], rebuild_flag=False, verbose_flag=True)")
