
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
import os
import shutil
import sqlite3
import subprocess
import time
from os.path import join, abspath
from shutil import rmtree
from tempfile import mkdtemp
from urllib.request import urlretrieve

from .utils import NonZeroExitStatus, friendly_exception, hashstr, run

BASE_DIR = os.environ.get('PLASH_DATA', '/tmp/plashdata')
TMP_DIR = join(BASE_DIR, 'tmp')
MNT_DIR = join(BASE_DIR, 'mnt')
BUILDS_DIR = join(BASE_DIR, 'builds')
ROTATE_LOG_SIZE = 4000

INDEXED_IMAGES= {
    'alpine3.3': 'https://images.linuxcontainers.org/images/alpine/3.3/amd64/default/20170811_17:50/rootfs.squashfs',
    'alpine3.4': 'https://images.linuxcontainers.org/images/alpine/3.4/amd64/default/20170811_17:50/rootfs.squashfs',
    'alpine3.5': 'https://images.linuxcontainers.org/images/alpine/3.5/amd64/default/20170811_17:50/rootfs.squashfs',
    'alpine3.6': 'https://images.linuxcontainers.org/images/alpine/3.6/amd64/default/20170811_17:50/rootfs.squashfs',
    'archlinux': 'https://images.linuxcontainers.org/images/archlinux/current/amd64/default/20170811_01:27/rootfs.squashfs',
    'artful': 'https://images.linuxcontainers.org/images/ubuntu/artful/amd64/default/20170811_09:29/rootfs.squashfs',
    'buster': 'https://images.linuxcontainers.org/images/debian/buster/amd64/default/20170811_19:07/rootfs.squashfs',
    'centos6': 'https://images.linuxcontainers.org/images/centos/6/amd64/default/20170811_02:16/rootfs.squashfs',
    'centos7': 'https://images.linuxcontainers.org/images/centos/7/amd64/default/20170811_02:16/rootfs.squashfs',
    'edge': 'https://images.linuxcontainers.org/images/alpine/edge/amd64/default/20170811_17:50/rootfs.squashfs',
    'fedora24': 'https://images.linuxcontainers.org/images/fedora/24/amd64/default/20170811_01:27/rootfs.squashfs',
    'fedora25': 'https://images.linuxcontainers.org/images/fedora/25/amd64/default/20170811_01:50/rootfs.squashfs',
    'fedora26': 'https://images.linuxcontainers.org/images/fedora/26/amd64/default/20170811_02:25/rootfs.squashfs',
    'gentoo': 'https://images.linuxcontainers.org/images/gentoo/current/amd64/default/20170811_14:56/rootfs.squashfs',
    'jessie': 'https://images.linuxcontainers.org/images/debian/jessie/amd64/default/20170810_23:47/rootfs.squashfs',
    'opensuse42.2': 'https://images.linuxcontainers.org/images/opensuse/42.2/amd64/default/20170811_00:53/rootfs.squashfs',
    'opensuse42.3': 'https://images.linuxcontainers.org/images/opensuse/42.3/amd64/default/20170811_00:53/rootfs.squashfs',
    'oracle6': 'https://images.linuxcontainers.org/images/oracle/6/amd64/default/20170811_11:40/rootfs.squashfs',
    'oracle7': 'https://images.linuxcontainers.org/images/oracle/7/amd64/default/20170811_11:40/rootfs.squashfs',
    'plamo5.x': 'https://images.linuxcontainers.org/images/plamo/5.x/amd64/default/20170810_21:36/rootfs.squashfs',
    'plamo6.x': 'https://images.linuxcontainers.org/images/plamo/6.x/amd64/default/20170810_21:36/rootfs.squashfs',
    'precise': 'https://images.linuxcontainers.org/images/ubuntu/precise/amd64/default/20170811_03:49/rootfs.squashfs',
    'sabayon': 'https://images.linuxcontainers.org/images/sabayon/current/amd64/default/20170811_07:38/rootfs.squashfs',
    'sid': 'https://images.linuxcontainers.org/images/debian/sid/amd64/default/20170811_19:07/rootfs.squashfs',
    'stretch': 'https://images.linuxcontainers.org/images/debian/stretch/amd64/default/20170811_19:07/rootfs.squashfs',
    'trusty': 'https://images.linuxcontainers.org/images/ubuntu/trusty/amd64/default/20170811_03:49/rootfs.squashfs',
    'ubuntu-core16': 'https://images.linuxcontainers.org/images/ubuntu-core/16/amd64/default/20170808_19:56/rootfs.squashfs',
    'wheezy': 'https://images.linuxcontainers.org/images/debian/wheezy/amd64/default/20170810_23:47/rootfs.squashfs',
    'xenial': 'https://images.linuxcontainers.org/images/ubuntu/xenial/amd64/default/20170811_03:49/rootfs.squashfs',
    'zesty': 'https://images.linuxcontainers.org/images/ubuntu/zesty/amd64/default/20170811_03:49/rootfs.squashfs'
}


class BuildError(Exception):
    pass

def pidsuffix():
    return '.{}'.format(os.getpid())

def touch(fname):

    # we should also trim the logfile sometimes
    f  = open(fname, 'a')
    f.close()


def prepare_data_dir(data_dir):
    for dir in [BASE_DIR, TMP_DIR, MNT_DIR, BUILDS_DIR]:
        try:
            os.mkdir(dir)
        except FileExistsError:
            pass


def prepare_rootfs(rootfs):
    # os.mkdir(join(mountpoint, 'proc'))
    # os.mkdir(join(mountpoint, 'sys'))
    # os.mkdir(join(mountpoint, 'dev'))
    # run(['mount', '--rbind', '/proc', join(rootfs, 'proc')])
    run(['mount', '-t', 'proc', 'proc', join(rootfs, 'proc')])
    run(['mount', '--bind', '/sys', join(rootfs, 'sys')])
    run(['mount', '--bind', '/dev', join(rootfs, 'dev')])
    run(['mount', '--bind', '/dev/pts', join(rootfs, 'dev', 'pts')])
    run(['mount', '--bind', '/dev/shm', join(rootfs, 'dev', 'shm')])
    # run(['mount', '--bind', '/tmp', join(rootfs, 'tmp')])
    run(['cp', '/etc/resolv.conf', join(rootfs, 'etc/resolv.conf')])


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

def layers_mount_payloads(layers):
    for layer in layers:
        squashfs = abspath(join(layer, '..', 'payload.squashfs'))
        mount_at = abspath(join(layer, '..', 'payload'))
        if not os.path.exists(squashfs):
            return
        # if os.listdir(join(layer))
        p = subprocess.Popen(['mount', squashfs, mount_at])
        exit = p.wait()
        print('=======', exit)

def staple_layer(layers, layer_cmd, rebuild=False):
    last_layer = layers[-1]
    layer_name = hashstr(' '.join(layers + [layer_cmd]).encode())
    final_child_dst = join(last_layer, 'children', layer_name)

    # layers_mount_payloads(layers)

    if not os.path.exists(final_child_dst) or rebuild:
        print('*** plash: building layer')
        new_child = mkdtemp(dir=TMP_DIR)
        new_layer = join(new_child, 'payload')
        os.mkdir(new_layer)
        os.mkdir(join(new_child, 'children'))
        touch(join(new_child, 'lastused'))

        mountpoint = mount_layers(layers=[join(i, 'payload') for i in layers], write_dir=new_layer)

        prepare_rootfs(mountpoint)
        p = subprocess.Popen(['chroot', mountpoint, 'sh', '-ce', layer_cmd], stdout=2, stderr=2)
        pid = p.wait()
        umount(mountpoint)
        assert pid == 0, 'Building returned non zero exit status'
        if pid != 0:
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
    layers_mount_payloads(layers)
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

def pull_base(image):
    try:
        image_url = INDEXED_IMAGES[image]
    except KeyError:
        raise ValueError('No such image, available: {}'.format(' '.join(sorted(INDEXED_IMAGES))))

    image_dir = join(BUILDS_DIR, join(hashstr(image_url.encode())))

    if not os.path.exists(image_dir):
        tmp_image_dir = mkdtemp(dir=TMP_DIR) # must be on same fs than BASE_DIR for rename to work
        os.mkdir(join(tmp_image_dir, 'children'))
        os.mkdir(join(tmp_image_dir, 'payload'))
        download_file = join(tmp_image_dir, 'payload.squashfs')
        print('Downloading image: ', end='', flush=True)
        urlretrieve(image_url, download_file, reporthook=reporthook)

        try:
            os.rename(tmp_image_dir, image_dir)
        except OSError as exc:
            if exc.errno == errno.ENOTEMPTY:
                print('*** plash: another process already pulled that image')
    
    return image_dir


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
        base,
        layer_commands,
        command,
        *,
        quiet_flag=False,
        verbose_flag=False,
        rebuild_flag=False,
        extra_mounts=[],
        build_only=False,
        skip_if_exists=True,
        extra_envs={}):

    # assert 0, layer_commands
    prepare_data_dir(BASE_DIR)
    base_dir = pull_base(base)
    layers = build(base_dir, layer_commands, rebuild_flag=rebuild_flag)

    if build_only:
        print('Build is ready')
    else:
        mountpoint = mount_layers([join(i, 'payload') for i in layers], mkdtemp(dir=TMP_DIR))
        last_layer = layers[-1]
        touch(join(last_layer, 'lastused')) # update the timestamp on this

        prepare_rootfs(mountpoint)
        os.chroot(mountpoint)

        os.chdir('/')
        with friendly_exception([FileNotFoundError]):
            os.execvpe(command[0], command, extra_envs)


# this as a separate script!

# cleanup_mounts()
# import sys; sys.exit(0)



if __name__ == '__main__':
    # print(staple_layer(['/tmp/data/layers/ubuntu'], 'touch a'))
    # print(staple_layer(staple_layer(['/tmp/data/layers/ubuntu'], 'touch a'), 'touch b'))
    # print(build('/tmp/data/layers/ubuntu', ['touch a', 'touch b', 'rm a']))
    execute('ubuntu:16.04', ['touch a', 'touch b', 'rm /a', 'apt-get update && apt-get install cowsay'], ['/usr/games/cowsay', 'hi'], rebuild_flag=False, verbose_flag=True)
    # call('ubuntu:16.04', ['touch ac', 'touch b', 'touch c'], ['/usr/games/cowsay', 'hi'], rebuild_flag=False, verbose_flag=True)
    # import cProfile
    # cProfile.run("call('ubuntu:16.04', ['touch a', 'touch b', 'rm /a', 'apt-get update && apt-get install cowsay'], ['/usr/games/cowsay', 'hi'], rebuild_flag=False, verbose_flag=True)")
