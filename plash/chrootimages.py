
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
from os.path import join
from shutil import rmtree
from tempfile import mkdtemp

from utils import NonZeroExitStatus, hashstr, run

BASE_DIR = os.environ.get('PLASH_DATA', '/tmp/plashdata')
TMP_DIR = join(BASE_DIR, 'tmp')
MNT_DIR = join(BASE_DIR, 'mnt')
BUILDS_DIR = join(BASE_DIR, 'builds')
ROTATE_LOG_SIZE = 4000


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
        touch(join(new_child, 'lastused'))

        mountpoint = mount(layers=(join(i, 'payload') for i in layers), write_dir=new_layer)

        prepare_rootfs(mountpoint)
        p = subprocess.Popen(['chroot', mountpoint, 'bash', '-ce', layer_cmd], stdout=2, stderr=2)
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
    
def mount(layers, write_dir):
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
    # print(cmd)
    run(cmd)
    return mountpoint

def pull_base(image):

    # normalize image name

    if not '/' in image:
        image = 'library/' + image
    if not ':' in image:
        image += ':latest'

    image_dir = join(BUILDS_DIR, join(hashstr('dockerregistry {}'.format(image).encode())))
    if not os.path.exists(image_dir):
        print('*** plash: preparing {}'.format(image))
        download_file = join(mkdtemp(), 'rootfs.tar.xz')
        tmp_image_dir = mkdtemp(dir=TMP_DIR) # must be on same fs than BASE_DIR for rename to work
        os.mkdir(join(tmp_image_dir, 'children'))
        os.mkdir(join(tmp_image_dir, 'payload'))

        # also doable with skopeo and oci-image-tool
        # run(['wget', 'https://us.images.linuxcontainers.org/images/ubuntu/artful/amd64/default/20170729_03:49/rootfs.tar.xz', '-O', download_file])
        # run(['tar', 'xf', download_file, '--exclude=./dev', '-C', join(tmp_image_dir, 'payload')])

        # subprocess.check_output(['docker', 'create', image])
        run(['bash', '-c', 'docker export $(docker create '+image+') | tar -C '+join(tmp_image_dir, 'payload')+' --exclude=/dev --exclude=/proc --exclude=/sys -xf -']) # command injection!! # excluding is not working!

        try:
            os.rename(tmp_image_dir, image_dir)
        except OSError as exc:
            if exc.errno == errno.ENOTEMPTY:
                print('*** plash: another process already pulled that image')

    return image_dir

def build(image, layers, *, quiet_flag=False, verbose_flag=False, rebuild_flag=False):
    base = [image]
    for layer in layers:
        base = staple_layer(base, layer, rebuild=rebuild_flag)
    return base


def call(base, layer_commands, cmd, *, quiet_flag=False, verbose_flag=False, rebuild_flag=False, extra_mounts=[], build_only=False):
    prepare_data_dir(BASE_DIR)
    base_dir = pull_base(base)
    layers = build(base_dir, layer_commands, rebuild_flag=rebuild_flag)
    mountpoint = mount([join(i, 'payload') for i in layers], mkdtemp(dir=TMP_DIR))
    last_layer = layers[-1]
    touch(join(last_layer, 'lastused')) # update the timestamp on this

    prepare_rootfs(mountpoint)
    print(mountpoint)
    os.chroot(mountpoint)

    os.chdir('/')
    os.execvpe(cmd[0], cmd, {'MYENV': 'myenvval'})

freespace()
import sys; sys.exit(0)

if __name__ == '__main__':
    # print(staple_layer(['/tmp/data/layers/ubuntu'], 'touch a'))
    # print(staple_layer(staple_layer(['/tmp/data/layers/ubuntu'], 'touch a'), 'touch b'))
    # print(build('/tmp/data/layers/ubuntu', ['touch a', 'touch b', 'rm a']))
    call('ubuntu:16.04', ['touch a', 'touch b', 'rm /a', 'apt-get update && apt-get install cowsay'], ['/usr/games/cowsay', 'hi'], rebuild_flag=False, verbose_flag=True)
    # call('ubuntu:16.04', ['touch ac', 'touch b', 'touch c'], ['/usr/games/cowsay', 'hi'], rebuild_flag=False, verbose_flag=True)
    # import cProfile
    # cProfile.run("call('ubuntu:16.04', ['touch a', 'touch b', 'rm /a', 'apt-get update && apt-get install cowsay'], ['/usr/games/cowsay', 'hi'], rebuild_flag=False, verbose_flag=True)")
