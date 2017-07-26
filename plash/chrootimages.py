import os
from tempfile import mkdtemp

from .utils import hashstr

class FSLock:
    # make it a context manager!!
    def acquire(self):
        try:
            os.mkdir(new_layer_lock)
        except IOError, exc:
            assert False, exc
            # check if its because already exists and then wait?
            if ITSBECAUSE_ITS_ALRWAdy_EXISTS:
                print('*** plash: waiting for the build of another process', newline=False)
                while True:
                    last_heartbeat =  os.stat(new_layer_heartbeat).st_mtime # gracefully fail if heartbeat file is not there yet
                    if time.time() - last_heartbeat > 3: # think about leapseconds?
                        print()
                        print('*** plash: current build process has no heartbeat, taking over and building', newline=False)
                        break
                    print('.', newline=False)
                    sleep(1)
            print() # print a newline

    def heartbeat(self)
        f = open(heartbeat, 'w'))
        f.close()

    def release(self):
        shutils(self._lockdir)


def staple_layer(layer_cmd):
    last_layer= self._layers[-1]
    layer_name = hashstr(' '.join(self._layers + [layer_cmd]))
    new_layer = os.path.join(last_layer, layer_name, 'payload')
    new_layer_lock = new_layer_path + '.lock'

    with layer_lock as FSLock(new_layer_lock)

        p = subprocess.Popen(['chroot', new_layer_path, 'sh', '-ce', build_dir])
        while True:
            pid = p.poll()
            if pid is None:
                layer_lock.heartbeat()
                sleep(1)
            else: # process exited
                break

        assert pid != 0, 'Building returned non zero exit status'
        if pid != 0:
            shutil.rmtree(new_layer)

    



def umount(mountpoint):
    'umount /my/mountpoint'

def fetch_base_image(image_name):
    pass


# exported
def build(image, layers, *, quiet_flag, verbose_flag, rebuild_flag, extra_mounts):
    base = image
    for layer in layers:
        base = staple_layer(base, layer)
    return base


def run(image, cmd):
    'mount image with tmp write branch'
    'chroot mounted_root cmd'

# [asdlfjladsf, asdfjlkasdf, adsfjkd, adsfj4]
