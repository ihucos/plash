import fcntl
import os
import shlex
import subprocess
import tempfile
from os import environ
from os.path import expanduser
from shlex import quote

from .utils import hashstr, rand


class BuildError(Exception):
    pass


home_directory = expanduser("~")


def docker_image_exists(ref):
    out = subprocess.check_output(
        ["docker", "images", "--quiet", "--filter",
         "reference={ref}".format(ref=ref)])
    return bool(out)

def docker_get_image_shell(image):
        etc_passwd = subprocess.check_output(['docker', 'run', image, 'cat', '/etc/passwd']).decode()
        shell = None
        for l in etc_passwd.splitlines():
            parts = l.split(':')
            if parts[0] == 'root':
                shell = parts[6]
        assert shell, 'could not determine which shell that os uses'
        return shell

def docker_run(image, cmd_with_args, extra_envs={}):

    args = [
        'docker',
        'run',
        '-ti',
        # '--expose', '8000',
        '--net=host', # does not bind the port on mac
        '--privileged',
        '--cap-add=ALL',
        '--workdir', os.getcwd(),
        '-v', '/dev:/dev',
        '-v', '/lib/modules:/lib/modules',
        '-v', '/var/run/docker.sock:/var/run/docker.sock',
        '-v', '/tmp:/tmp',
        '-v', '{}:{}'.format(home_directory, home_directory),
        '--rm',
        image,
    ] + list(cmd_with_args)

    for env, env_val in dict(environ, **extra_envs).items():
        if env not in ['PATH', 'LC_ALL', 'LANG']: # blacklist more envs
        # if env not in ['PATH']: # blacklist more envs
            args.insert(2, '-e')
            args.insert(3, '{}={}'.format(env, shlex.quote(env_val)))

    # return subprocess.Popen(args).wait()
    os.execvpe('docker', args, os.environ)



class BaseDockerBuildable:

    @classmethod
    def create(cls, base_iamge, build_commands):
        class TmpBuildable(cls):
            def get_base_image_name(self):
                return base_iamge
            def get_build_commands(self):
                return build_commands
        return TmpBuildable()

    def image_ready(self):
        return docker_image_exists(self.get_image_name())

    def get_base_image_name(self):
        raise NotImplementedError('you lazy person')

    def get_build_commands(self):
        raise NotImplementedError('you lazy person')


class DockerBuildable(BaseDockerBuildable):
   
    def get_image_name(self):
        h = hashstr('{}-{}'.format(
            self.get_base_image_name(), self.get_build_commands()).encode())
        return 'plash-{}'.format(h)

    def build(self, quiet=True, verbose=False, extra_mounts=[], shell='bash'):
        rand_name = rand()
        cmds = self.get_build_commands()
        new_image_name = self.get_image_name()

        fd_3_is_open = not fcntl.fcntl(3, fcntl.F_GETFD)
        if fd_3_is_open:
            handle = 3
            fname = None
        elif quiet:
            handle, fname = tempfile.mkstemp(suffix='__plash_build.log')
        else:
            handle = 2 # stderr
            fname = None

        args = [
            'docker',
            'run',
            '-ti',
            # '-v', '/Users/iraehueckcosta/.aptcache:/var/cache/apt/archives', # cache apt packages -- implement that later!
            '--net=host', # does not bind the port on mac
            '--privileged',
            '--cap-add=ALL',
            '-v', '/dev:/dev',
            '-v', '/lib/modules:/lib/modules',
            '--name',
            rand_name, self.get_base_image_name(),
            # 'bash', '-cx', cmds], # with bash debug script
            shell, '-ce'+('x' if verbose else ''), cmds]

        for (from_, to) in extra_mounts:
            args.insert(2, '-v')
            args.insert(3, '{}:{}:ro'.format(from_, to))

        exit = subprocess.Popen(args, # with bash debug script
                                stderr=handle, stdout=handle,
        ).wait()


        if not exit == 0 and fname:
            dbgcmd = 'tail -n 5 {}'.format(shlex.quote(fname))
            # print('$ ' + dbgcmd)
            subprocess.check_call(dbgcmd, shell=True)
            print()
            print('Build log at: ' + fname)
        
        if not exit == 0:
            raise BuildError('building returned exit status {}'.format(exit))
        if exit == 0 and fname:
            os.remove(fname)

        # get cotnainer id
        container_id = subprocess.check_output(
        ['docker', 'ps', '--all', '--quiet', '--filter', 'name={}'.format(rand_name)])

        container_id, = container_id.splitlines()

        # create image out of the container
        from time import sleep
        sleep(0.2) # race condition in docker?
        subprocess.check_output(['docker', 'commit', container_id, new_image_name])

        # remove the container to save space
        subprocess.check_output(['docker', 'rm', container_id])


class LayeredDockerBuildable(BaseDockerBuildable):

    def get_base_image_name(self):
        raise NotImplementedError('you lazy person')

    def get_build_commands(self):
        '''
        Returns a list of commands for each layer
        '''
        raise NotImplementedError('you lazy person')

    def _build(self, meth, *args, **kw):
        parent_img = self.get_base_image_name()
        for layer_cmd in self.get_build_commands():
            buildable = DockerBuildable.create(parent_img, layer_cmd)
            if meth == 'build':

                # dirty abstraction, "but its just a print"
                print('*** plash is running --layer')

                buildable.build(*args, **kw)
            elif meth == 'get_image_name':
                pass
            else:
                raise TypeError('Invalid meth')
            parent_img = buildable.get_image_name()
        return parent_img

    def build(self, *args, **kw):
        self._build('build', *args, **kw)

    def get_image_name(self):
        return self._build('get_image_name')

if __name__ == "__main__":
    b = LayeredDockerBuildable.create('ubuntu', ['touch /a', 'touch /b'])
    b.build(quiet=False)
    print
    b = LayeredDockerBuildable.create('ubuntu', ['touch /a', 'touch /d'])
    b.build(quiet=False)
