import errno
import os.path
import shutil
import subprocess
from os.path import join
from sys import argv
from tempfile import mkdtemp

from plash.utils import hashstr, run

TMP_DIR = '/tmp'


class Container:

    def __init__(self, container_id):
        self._layer_ids = container_id.split(':')

    def hash_cmd(self, cmd):
        return hashstr(cmd)[:12]

    def get_layer_paths(self):
        lp = ["/var/lib/plash/builds/{}".format(self._layer_ids[0])]
        for ci in self._layer_ids[1:]:
            lp.append(lp[-1] + '/children/' + ci)
        # print(lp)
        return lp

    def invalidate(self):
        pass

    def mount_rootfs(self, *, mountpoint, write_dir=None, workdir=None):
        if write_dir == None:
            write_dir = mkdtemp(dir=TMP_DIR)
        if workdir == None:
            workdir = mkdtemp(dir=TMP_DIR)
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

        self.mount_rootfs(mountpoint=new_layer)

        self.prepare_chroot(new_layer)

        if not os.fork():
            os.chroot(new_layer)
            # from time import sleep
            # sleep(5)
            shell = 'sh'
            os.execvpe(shell, [shell, '-ce', cmd], os.environ) # maybe isolate envs better?
        child_pid, child_exit = os.wait()

        run(["umount", "--recursive", new_layer])

        if child_exit != 0:
            print("*** plash: build failed with exit status: {}".format(child_exit))
            shutil.rmtree(new_child)
            sys.exit(1)


        last_layer = self.get_layer_paths()[-1]
        layer_hash = self.hash_cmd(cmd.encode())
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
        layer_hash = self.hash_cmd(cmd.encode())
        last_layer = self.get_layer_paths()[-1]
        final_child_dst = join(last_layer, 'children', layer_hash)
        return os.path.exists(final_child_dst)


    def add_layer(self, cmd):
        if not self.is_builded(cmd):
            return self.build_layer(cmd)
        else:
            print('*** plash: cached layer')

    def create_runnable(self, file, layers_cmd, executable, *, verbose_flag=False):
        for cmd in layers_cmd:
            self.add_layer(cmd)

        mp = mkdtemp(dir=TMP_DIR)
        self.mount_rootfs(mountpoint=mp)

        assert False, mp # create runnable from here



c = Container('zesty')
c.create_runnable('/tmp/mytest', ['touch a', 'touch b'], "python3")
