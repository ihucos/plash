from plash.utils import hashstr
from os.path import join
import os.path


class Container:

    def __init__(self, layers_ids):
        self._layer_ids = []

    def get_layer_paths(self):
        lp = ["/var/lib/plash/builds/{}".format(self._container_id[0])]
        for ci in self._container_id[1:]:
            lp.append(lp[-1] + 'children/' + ci
        return lp

    def invalidate(self, self):
          pass

    def mount_rootfs(self, write_dir, mountpoint):
        mountpoint = mkdtemp(dir=MNT_DIR, suffix=pidsuffix()) # save pid so we can unmout it when that pid dies
        workdir = mkdtemp(dir=MNT_DIR)
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

        self.mount_rootfs(workdir=mkdtemp(dir=TMP_DIR), mounpoint=new_layer)

        self.prepare_chroot(mounpoint)

        # do that with fork, chroot and exec?
        p = subprocess.Popen(
            [argv[0],
             'chroot',
             '--cow', new_layer,
             mountpoint,
             '--', 'sh', '-ce', cmd], stdout=2, stderr=2)
        exit = p.wait()
        run("umount", mountpoint)
        assert exit == 0, 'Building returned non zero exit status'
        if exit != 0:
            print('non zero exit status code when building')
            shutil.rmtree(build_at)
            assert False

        layer_hash = hashstr(' '.join(layers + [layer_cmd]).encode())
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
        last_layer = self.get_layers()[-1]
        layer_hash = hashstr(' '.join(layers + [layer_cmd]).encode())
        final_child_dst = join(last_layer, 'children', layer_hash)
        return os.path.exists(final_child_dst)


    def add_layer(self, cmd):
        if not self.is_builded(cmd)
              return build_layer(cmd)
        else:
            print('*** plash: cached layer'.format(layer_hash))

    def create_runnable(file, layers_cmd, *, verbose_flag=False):
        for cmd in layers_cmd:
            self.build_layer(cmd)

        mp = mkdtemp(dir=TMP_DIR)
        self.mount_rootfs(workdir=mkdtemp(dir=TMP_DIR), mounpoint=mp)

        assert False, mp # create runnable from here




create_runnable('/tmp/mytest', ["ubuntu"], ['touch a'], "python3")
