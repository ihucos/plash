import subprocess

from utils import hashstr, rand

class BuildError(Exception):
    pass


class DockerBuildable:

    def get_image_name(self):
        h = hashstr('{}-{}'.format(
            self.get_base_image_name(), self.get_build_commands()).encode())
        return 'packy-{}'.format(h)

    def get_base_image_name(self):
        raise NotImplementedError('you lazy person')

    def get_build_commands(self):
        raise NotImplementedError('you lazy person')

    def image_exists(self, ref):
        # return False
        out = subprocess.check_output(
            ["docker", "images", "--quiet", "--filter",
             "reference={ref}".format(ref=ref)])
        return bool(out)
    
    def ensure_builded(self, quiet=False):
        if not self.image_exists(self.get_image_name()):
            self.build(quiet)

    def build(self, quiet=True):
        rand_name = rand()
        cmds = self.get_build_commands()
        new_image_name = self.get_image_name()

        quiet_kw = {'stderr': subprocess.DEVNULL, 'stdout': subprocess.DEVNULL}
        exit = subprocess.Popen([
            'docker',
            'run',
            '-ti',
            # '-v', '/Users/iraehueckcosta/.aptcache:/var/cache/apt/archives', # cache apt packages -- implement that later!
            '-v', '/:/.host_fs_do_not_use', # cache apt packages -- implement that later!
            '--net=host', # does not bind the port on mac
            '--privileged',
            '--cap-add=ALL',
            '-v', '/dev:/dev',
            '-v', '/lib/modules:/lib/modules',
            '--name',
            rand_name, self.get_base_image_name(),
            # 'bash', '-cx', cmds], # with bash debug script
            'bash', '-c', cmds],
        **(quiet_kw if quiet else {})).wait()
        if not exit == 0:
            raise BuildError('building returned exit status {}'.format(exit))

        # get cotnainer id
        container_id = subprocess.check_output(
        ['docker', 'ps', '--all', '--quiet', '--filter', 'name={}'.format(rand_name)])

        container_id, = container_id.splitlines()

        # create image out of the container
        from time import sleep
        sleep(0.2) # race condition in docker?
        exit = subprocess.Popen(['docker', 'commit', container_id, new_image_name], **quiet_kw).wait()
        assert exit == 0

        # remove the container to save space
        exit = subprocess.Popen(['docker', 'rm', container_id], **quiet_kw).wait()
        assert exit == 0
