import os
import shutil
import subprocess
from tempfile import mkdtemp

from utils import hashstr

TMP_DIR = '/tmp/data/tmp'
MNT_DIR = '/tmp/data/mnt'


def staple_layer(layers, layer_cmd):
    last_layer = layers[-1]
    layer_name = hashstr(' '.join(layers + [layer_cmd]).encode())

    # os.mkdir(os.path.join(last_layer, layer_name)) # what if already exists?
    try:
        os.mkdir(os.path.join(last_layer, 'children', layer_name))
    except FileExistsError:
        pass
    try:
        os.mkdir(os.path.join(last_layer, 'children', layer_name, 'payload'))
    except FileExistsError:
        pass
    try:
        os.mkdir(os.path.join(last_layer, 'children', layer_name, 'children'))
    except FileExistsError:
        pass

    new_layer = os.path.join(last_layer, 'children', layer_name, 'payload')

    try:
        os.mkdir(new_layer)
    except FileExistsError:
        pass
    build_at = mkdtemp(dir=TMP_DIR)
    mountpoint = mount(layers=(os.path.join(i, 'payload') for i in layers), write_dir=build_at)

    p = subprocess.Popen(['chroot', mountpoint, 'bash', '-ce', layer_cmd])
    pid = p.wait()
    umount(mountpoint)
    assert pid == 0, 'Building returned non zero exit status'
    if pid != 0:
        print('non zero exit status code when building')
        shutil.rmtree(build_at)
        assert False
    else:
        try:
            os.rename(build_at, new_layer)
        except OSError as exc:
            raise
            # check if it already exists, if yes cleanup (rmtree) and continue
    return layers + [os.path.abspath(os.path.join(new_layer, '..'))]


def umount(mountpoint):
    p = subprocess.Popen(['umount', mountpoint])
    exit = p.wait()
    assert exit == 0

def mount(layers, write_dir):
    mountpoint = mkdtemp(dir=MNT_DIR)

    p = subprocess.Popen([
        'mount',
        '-t',
        'aufs',
        '-o',
        'dirs={write_dir}=rw:{mountpoint}'.format(
            write_dir=write_dir,
            mountpoint=':'.join(i + '=ro' for i in layers)),
        'none',
        mountpoint])
    exit = p.wait()
    assert exit == 0, 'mounting got non zero exit status'
    return mountpoint

def fetch_base_image(image_name):
    pass


# exported
def build(image, layers, *, quiet_flag=False, verbose_flag=False, rebuild_flag=False, extra_mounts=[]):
    base = [image]
    for layer in layers:
        base = staple_layer(base, layer)
    return base


def run(layers, cmd):
    mountpoint = mount([os.path.join(i, 'payload') for i in layers], mkdtemp(dir=TMP_DIR))
    os.chroot(mountpoint)
    os.chdir('/')
    os.execvpe(cmd[0], cmd, {'MYENV': 'myenvval'})


if __name__ == '__main__':
    # print(staple_layer(['/tmp/data/layers/ubuntu'], 'touch a'))
    # print(staple_layer(staple_layer(['/tmp/data/layers/ubuntu'], 'touch a'), 'touch b'))
    # print(build('/tmp/data/layers/ubuntu', ['touch a', 'touch b', 'rm a']))
    run(build('/tmp/data/layers/ubuntu', ['touch a', 'touch b', 'rm /a']), ['/bin/bash'])

# [asdlfjladsf, asdfjlkasdf, adsfjkd, adsfj4]
