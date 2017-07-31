
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
from tempfile import mkdtemp

from utils import hashstr, run, NonZeroExitStatus

BASE_DIR = os.environ.get('PLASH_DATA', '/tmp/plashdata')
TMP_DIR = join(BASE_DIR, 'tmp')
MNT_DIR = join(BASE_DIR, 'mnt')
BUILDS_DIR = join(BASE_DIR, 'builds')
ROTATE_LOG_SIZE = 4000

def log_usage(layer_name):

    # we should also trim the logfile sometimes
    with open('/tmp/usage.log', 'a') as f:
        f.write('{} {}\n'.format(int(round(time.time())), layer_name))


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
    mountpoint = mkdtemp(dir=MNT_DIR, suffix='.{}'.format(os.getpid())) # save pid so we can unmout it when that pid dies
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

def fetch_base_image(image_name):
    pass


def pull_base(image):

    # normalize image name

    if not '/' in image:
        image = 'library/' + image
    if not ':' in image:
        image += ':latest'

    image_dir = join(BUILDS_DIR, join(hashstr('dockerregistry {}'.format(image).encode())))
    if not os.path.exists(image_dir):
        print('*** plash: preparing {}'.format(image))
        tmpdir = mkdtemp()
        download_file = join(tmpdir, 'rootfs.tar.xz')
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


def call(base, layer_commands, cmd, *, quiet_flag=False, verbose_flag=False, rebuild_flag=False, extra_mounts=[]):
    prepare_data_dir(BASE_DIR)
    base_dir = pull_base(base)
    layers = build(base_dir, layer_commands, rebuild_flag=rebuild_flag)
    mountpoint = mount([join(i, 'payload') for i in layers], mkdtemp(dir=TMP_DIR))
    log_usage(layers[-1].split('/')[-1])

    prepare_rootfs(mountpoint)
    print(mountpoint)
    os.chroot(mountpoint)

    os.chdir('/')
    os.execvpe(cmd[0], cmd, {'MYENV': 'myenvval'})

# this as a separate script!
def cleanup_mounts():
    with open('/proc/mounts') as f:
        mounts = f.readlines()

    umount_this = []
    mnt_dir_depth = MNT_DIR.count('/')
    for line in mounts:
        device, mountpoint, fs_type, ro_or_rw, dumm1, dumm2 = line.split()
        if mountpoint.startswith(MNT_DIR):
            mountpoint_depth = mountpoint.count('/')
            if mountpoint_depth - mnt_dir_depth == 1: # if it is the root mount of that container
                try:
                    pid = mountpoint.split('.')[-1]
                    pid = int(pid)
                except ValueError:
                    continue
                if not os.path.exists(join('/proc', str(pid))):
                    umount_this.append((mountpoint, pid))

    
    # umount in this pid creation order BUG BUG BUG pids are not always ascending
    for mp, _ in sorted(umount_this, key=lambda i: -i[1]): # use itemgetter from itertools
        print('umounting {}'.format(mp))
        umount(mp)

# cleanup_mounts()
# import sys; sys.exit(0)


if __name__ == '__main__':
    # print(staple_layer(['/tmp/data/layers/ubuntu'], 'touch a'))
    # print(staple_layer(staple_layer(['/tmp/data/layers/ubuntu'], 'touch a'), 'touch b'))
    # print(build('/tmp/data/layers/ubuntu', ['touch a', 'touch b', 'rm a']))
    call('ubuntu:16.04', ['touch a', 'touch b', 'rm /a'], ['/bin/bash'], rebuild_flag=True, verbose_flag=True)
