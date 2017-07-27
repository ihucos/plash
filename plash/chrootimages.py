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
    os.mkdir(os.path.join(last_layer, layer_name)) # what if already exists?
    new_layer = os.path.abspath(os.path.join(last_layer, '..', layer_name, 'layer'))
    try:
        os.mkdir(os.path.abspath(os.path.join(new_layer, '..')))
    except FileExistsError:
        pass
    try:
        os.mkdir(new_layer)
    except FileExistsError:
        pass
    build_at = mkdtemp(dir=TMP_DIR)
    mountpoint = mount(layers=layers, write_dir=build_at)

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
    return layers + [new_layer]


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
def build(image, layers, *, quiet_flag, verbose_flag, rebuild_flag, extra_mounts):
    base = image
    for layer in layers:
        base = staple_layer(base, layer)
    return base


def run(layers, cmd):
    mountpoint = mount(layers, mkdtemp())
    os.chroot(mountpoint)
    os.execvpe(cmd[0], cmd, {'MYENV', 'myenvval'})


if __name__ == '__main__':
    print(staple_layer(['/tmp/data/layers/ubuntu/rootfs'], 'touch a'))
    # print(staple_layer(staple_layer(['/tmp/data/layers/ubuntu/rootfs'], 'touch a'), 'touch b'))
        

# [asdlfjladsf, asdfjlkasdf, adsfjkd, adsfj4]
