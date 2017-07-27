import errno
import os
import shutil
import subprocess
from tempfile import mkdtemp

from utils import hashstr

TMP_DIR = '/tmp/data/tmp'
MNT_DIR = '/tmp/data/mnt'


def staple_layer(layers, layer_cmd, rebuild=False):
    last_layer = layers[-1]
    layer_name = hashstr(' '.join(layers + [layer_cmd]).encode())
    final_child_dst = os.path.join(last_layer, 'children', layer_name)

    if not os.path.exists(final_child_dst) or rebuild:
        new_child = mkdtemp(dir=TMP_DIR)
        new_layer = os.path.join(new_child, 'payload')
        os.mkdir(new_layer)
        os.mkdir(os.path.join(new_child, 'children'))

        mountpoint = mount(layers=(os.path.join(i, 'payload') for i in layers), write_dir=new_layer)

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
                        os.rename(final_child_dst, os.path.join(mkdtemp(dir=TMP_DIR)))
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
    p = subprocess.Popen(['umount', mountpoint])
    exit = p.wait()
    assert exit == 0

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
    p = subprocess.Popen(cmd)
    exit = p.wait()
    assert exit == 0, 'mounting got non zero exit status'
    return mountpoint

def fetch_base_image(image_name):
    pass


# exported
def build(image, layers, *, quiet_flag=False, verbose_flag=False, rebuild_flag=False):
    base = [image]
    for layer in layers:
        base = staple_layer(base, layer, rebuild=rebuild_flag)
    return base


def run(base, layer_commands, cmd, *, quiet_flag=False, verbose_flag=False, rebuild_flag=False, extra_mounts=[]):
    layers = build(base, layer_commands, rebuild_flag=True)
    mountpoint = mount([os.path.join(i, 'payload') for i in layers], mkdtemp(dir=TMP_DIR))
    os.chroot(mountpoint)
    os.chdir('/')
    os.execvpe(cmd[0], cmd, {'MYENV': 'myenvval'})


if __name__ == '__main__':
    # print(staple_layer(['/tmp/data/layers/ubuntu'], 'touch a'))
    # print(staple_layer(staple_layer(['/tmp/data/layers/ubuntu'], 'touch a'), 'touch b'))
    # print(build('/tmp/data/layers/ubuntu', ['touch a', 'touch b', 'rm a']))
    run('/tmp/data/layers/ubuntu', ['touch a', 'touch b', 'rm /a'], ['/bin/bash'], rebuild_flag=True,)

# [asdlfjladsf, asdfjlkasdf, adsfjkd, adsfj4]
