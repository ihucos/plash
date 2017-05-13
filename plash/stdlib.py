import hashlib
import os
import shlex
import stat
import subprocess
import sys
import uuid
from base64 import b64encode

import yaml

from .actionutils import Action, ArgError, action, eval
from .utils import hashstr, rand


@action('pdb')
def pdb():
    import pdb
    pdb.set_trace()

# class Include(Action):
#     def handle_arg(self, file):
#         fname = os.path.realpath(fname)
#         with open(fname) as f:
#             config = f.read()
#         loaded = yaml.load(config)
#         return self.eval(loaded)


class LayeEach(Action):
    name = 'with-layers'
    debug = False

    def __call__(self, command, *args):
        lst = []
        for arg in args:
            lst.append([command, arg])
            lst.append(['layer'])
        return self.eval(lst)

class Inline(Action):
    name = 'inline'
    def handle_arg(self, arg):
        return arg

@action('run')
def run(*args):
    return ' '.join(args)


class Frun(Action):
    name = 'pun'

    def __call__(self, *args):
        init = []
        cleanup = []
        replaced_args = []
        for arg in args:
            if arg.startswith('{') and arg.endswith('}'):
                path = arg[1:-1]
                if not os.path.exists(path):
                    raise ArgError('Path {} does not exist'.format(path))
                p = subprocess.Popen(['tar', '-c', path],
                                     stderr=subprocess.DEVNULL, stdout=subprocess.PIPE)
                assert p.wait() == 0
                out = p.stdout.read()
                baseout = b64encode(out).decode()
                outpath = os.path.join('/tmp', hashstr(out))
                init.append('mkdir {}'.format(outpath))
                init.append('echo {baseout} | base64 --decode | tar -C {out} -x --strip-components 1'.format(
                    out=outpath, baseout=baseout))
                replaced_args.append(os.path.join(outpath, os.path.basename(path)))
                # replaced_args.append(outpath)
            else:
                replaced_args.append(arg)

        return '{}\n{}\n'.format(
            ' && '.join(init), ' '.join(replaced_args), ' && '.join(cleanup))

# class Home(Action):

#     def __call__(self):
#         return self.eval([['include', path.join(home_path, '.plash.yaml')]])


# print(eval([['layer-each', 'inline', 'hi', 'ho']]))






class PackageManager(Action):
    pre_install = None
    post_install = None
    single_packge_per_install = False

    def __call__(self, *packages):
        cmds = []
        if self.pre_install:
            cmds.append(self.pre_install)
        if not self.single_packge_per_install:
            ps = ' '.join(shlex.quote(p) for p in packages)
            cmds.append(self.install.format(ps))
        else:
            for p in packages:
                cmds.append(self.install.format(p))
        if self.post_install:
            cmds.append(self.post_install)
        return ' && '.join(cmds)


class Apt(PackageManager):
    short_name = 'a'
    pre_install = 'apt-get update'
    install = 'apt-get install -y {}'


class AddAptRepository(PackageManager):
    name = 'add-apt-repository'
    install = 'add-apt-repository -y {}'

class AptByCommandName(Action):
    name = 'apt-from-command'
    def __call__(self, command):
        p = subprocess.Popen([
            sys.argv[0],
            '--ubuntu',
            '--apt', 'command-not-found',
            '--quiet',
            '--',
            '/usr/lib/command-not-found',
            '--ignore-installed',
            '--no-failure-msg',
            command],
            stderr=subprocess.PIPE,
            stdout=subprocess.PIPE,
        )
        p.wait()
        out = p.stdout.read()
        if not out or b'No command' in out:
            raise SystemExit('Command {} not found'.format(command))
        line2 = out.splitlines()[1]
        package = line2.split()[-1]

        return str(Apt.call(package.decode()))


class RebuildWhenChanged(Action):
    name = 'rebuild-when-changed'

    def __call__(self, *paths):
        all_files = []
        for path in paths:
            if os.path.isdir(path):
                all_files.extend(self._extract_files(path))
            else:
                all_files.append(path)
        
        hasher = hashlib.sha1()
        for fname in sorted(all_files):
            perm = str(oct(stat.S_IMODE(os.lstat(fname).st_mode))
                      ).encode()
            with open(fname, 'rb') as f:
                fread = f.read()
            hasher.update(fname.encode())
            hasher.update(perm)
            hasher.update(self._hash_str(fread))

        hash = hasher.hexdigest() 
        return "echo 'rebuild-when-changed: hash {}'".format(hash)

    def _extract_files(self, dir):
        for (dirpath, dirnames, filenames) in os.walk(dir):
            for filename in filenames:
                fname = os.sep.join([dirpath, filename])
                yield fname


    
    def _hash_str(self, stri):
        hasher = hashlib.sha1()
        hasher.update(stri)
        return hasher.digest()



class Apk(PackageManager):
    pre_install = 'apk update'
    install = 'apk add {}'


class Yum(PackageManager):
    short_name = 'y'
    install = 'yum install -y {}'


class Pip(PackageManager):
    short_name = 'p'
    install = 'pip install {}'


class Npm(PackageManager):
    install = 'npm install -g {}'


class FileCommand(Action):

    def __call__(self, fname):
        with open(fname) as f:
            encoded = b64encode(f.read().encode())
        inline_file = '<(echo {} | base64 --decode)'.format(
            encoded.decode())
        return self.cmd.format(inline_file)

class PipRequirements(FileCommand):
    name = 'pip-requirements'
    cmd = 'pip install -r {}'

class Execute(FileCommand):
    cmd = 'cp {} /tmp/file && chmod +x /tmp/file && ./tmp/file && rm /tmp/file'

class Interactive(Action):
    def __call__(self, name=''):
        return "echo 'Exit shell when ready' && bash && : modifier name is {}".format(
            shlex.quote(name))


class Mount(Action):
    def handle_arg(self, mountpoint):
        mountpoint = os.path.realpath(mountpoint)
        if not os.path.isdir(mountpoint):
            raise ArgError('{} is not a directory'.format(mountpoint))
        from_ = os.path.join('/.host_fs_do_not_use', mountpoint.lstrip('/'))
        cmd = "mkdir -p {dst} && mount --bind {src} {dst}".format(
            src=shlex.quote(from_),
            dst=shlex.quote(mountpoint))
        return cmd

class Pwd(Action):
    def __call__(self, pwd):
        return 'cd {}'.format(os.path.realpath(pwd))


class ImportEnv(Action):
    name = 'import-prefixed-envs'

    def handle_arg(self, env):
        val = os.environ.get(env)
        if val is None:
            raise ArgError('No such env in host: {}'.format(env))
        return 'HOST_{}={}'.format(env, shlex.quote(val))

class Emerge(PackageManager):
    install = 'emerge {}'

class Home(Action):
    name = 'rc'
    short_name = 'r'
    def __call__(self, *args):
        path = os.path.join(os.path.expanduser("~"), '.plash.yaml')
        return Include.call(path) + [Layer.call()]


class BustCashe(Action):
    name = 'bustcache'

    def __call__(self):
        return ': bust cache with {}'.format(uuid.uuid4()) 


@action('pkg', debug=False)
def pkg(*packages):
    raise ArgError('you need to ":set-pkg <package-manager>" to use pkg')

@action('set-pkg')
def set_pkg(pm):
    @action('pkg', debug=False)
    def pkg(*packages):
        return eval([[pm] + list(packages)])
    return ':'


@action('bootstrap')
def bootstrap(os):
    os_base = os.split(':')[0]

    if os_base == 'ubuntu':
        return eval([
            ['set-pkg', 'apt'],
            ['inline', 'rm /etc/apt/apt.conf.d/docker-clean'],
            ['pkg', 'python-pip', 'npm', 'software-properties-common'],
            ['layer']
        ])

    elif os_base == 'debian':
        return eval([
            ['set-pkg', 'apt'],
            ['pkg', 'python-pip', 'npm', 'software-properties-common'],
            ['layer']
        ])

    elif os_base == 'centos':
        return eval([
            ['set-pkg', 'yum'],
            ['pkg', 'epel-release', 'npm', 'python-pip'],
            ['layer']
        ])

    elif os_base == 'alpine':
        return eval([
            ['set-pkg', 'apk'],
        ])

    elif 'gentoo' in os_base:
        return eval([
            ['set-pkg', 'emerge'],
            ['pkg', 'dev-python/pip'],
            ['layer']
        ])
    else:
        return "echo no recipe to bootstrap: {}".format(os)


# @action('rebuild-every')
# def rebuild_every(value, unit):
# ret
# class RebuildEvery(Action):
#     name = 'rebuild-every'

#     def __call__(self, value, unit):
#         """
#         for example 
#         --rebuild-every 2 weeks
#         --rebuild-every 1 year

#         an then in the very end of a config

#         ...
#         - layer
#         - rebuild-every: 2 months
#         - apt: update
#         - apt: upgrade -y

#         the implementation prints the next time there will be a rebuild
#         rebuilds are preconfigured, its time.time() % EVERY_SECONDS
#         Later add an jitter 

#         """
