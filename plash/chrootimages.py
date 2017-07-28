import errno
import os
import shutil
import sqlite3
import subprocess
import time
from os.path import join
from tempfile import mkdtemp

from utils import hashstr, run

BASE_DIR = '/tmp/data'
TMP_DIR = join(BASE_DIR, 'tmp')
MNT_DIR = join(BASE_DIR, 'mnt')
BUILDS_DIR = join(BASE_DIR, 'layers')
ROTATE_LOG_SIZE = 4000

def log_usage(layer_name):

    # we should also trim the logfile sometimes
    with open('/tmp/usage.log', 'a') as f:
        f.write('{} {}\n'.format(int(round(time.time())), layer_name))

def staple_layer(layers, layer_cmd, rebuild=False):
    last_layer = layers[-1]
    layer_name = hashstr(' '.join(layers + [layer_cmd]).encode())
    final_child_dst = join(last_layer, 'children', layer_name)

    if not os.path.exists(final_child_dst) or rebuild:
        new_child = mkdtemp(dir=TMP_DIR)
        new_layer = join(new_child, 'payload')
        os.mkdir(new_layer)
        os.mkdir(join(new_child, 'children'))

        mountpoint = mount(layers=(join(i, 'payload') for i in layers), write_dir=new_layer)

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
    run(['umount', mountpoint])
    
def mount(layers, write_dir):
    mountpoint = mkdtemp(dir=MNT_DIR)
    cmd = [
        'mount',
        '-t',
        'aufs',
        '-o',
        'dirs={write_dir}=rw:{mountpoint}'.format(
            write_dir=write_dir,
            mountpoint=':'.join(i + '=ro' for i in layers)),
        'none',
        mountpoint]
    # print(cmd)
    run(cmd)
    return mountpoint

def fetch_base_image(image_name):
    pass


# exported
def build(image, layers, *, quiet_flag=False, verbose_flag=False, rebuild_flag=False):
    base = [image]
    for layer in layers:
        base = staple_layer(base, layer, rebuild=rebuild_flag)
    return base


def call(base, layer_commands, cmd, *, quiet_flag=False, verbose_flag=False, rebuild_flag=False, extra_mounts=[]):
    base_dir = join(BUILDS_DIR, base)
    layers = build(base_dir, layer_commands, rebuild_flag=True)
    mountpoint = mount([join(i, 'payload') for i in layers], mkdtemp(dir=TMP_DIR))
    log_usage(layers[-1].split('/')[-1])
    os.chroot(mountpoint)
    os.chdir('/')
    os.execvpe(cmd[0], cmd, {'MYENV': 'myenvval'})


if __name__ == '__main__':
    # print(staple_layer(['/tmp/data/layers/ubuntu'], 'touch a'))
    # print(staple_layer(staple_layer(['/tmp/data/layers/ubuntu'], 'touch a'), 'touch b'))
    # print(build('/tmp/data/layers/ubuntu', ['touch a', 'touch b', 'rm a']))
    call('ubuntu', ['touch a', 'touch b', 'rm /a'], ['/bin/bash'], rebuild_flag=True,)
