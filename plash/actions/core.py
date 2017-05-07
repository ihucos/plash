
import hashlib
import os
import shlex
import stat
import subprocess
from base64 import b64encode

import yaml

from actions.base import actions as ACTIONS
from actions.base import Action, ArgError


class LAYER:
    pass
LAYER = LAYER()

# should not be here
def tokenize_actions(actions):
    tokens = []
    for action_name, args in actions:
        action = ACTIONS[action_name]
        cmd = action.friendly_call(*args)
        if isinstance(cmd, str) or cmd is LAYER:
            tokens.append(cmd)
        elif isinstance(cmd, list):
            tokens.extend(cmd)
        else:
            raise ValueError('bad action return value')
    return tokens




class IncludeError(Exception):
    pass

class PackageManager(Action):
    pre_install = None
    # post_install = None

    def __call__(self, *packages):
        cmds = []
        if self.pre_install:
            cmds.append(self.pre_install)
        for p in packages:
            cmds.append(self.install.format(p))
        # if self.post_install:
        #     cmds.append(self.post_install)
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
            os.path.join(os.path.dirname(os.path.dirname(__file__)), 'plash'),
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
        if not out:
            raise SystemExit('Command {} not found'.format(command))
        print(out)
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

class Eval(Action):
    def __call__(self, *stris):
        return ' && '.join(stris)

class Interactive(Action):
    def __call__(self, name):
        return "echo 'Exit shell when ready' && bash && : modifier name is {}".format(
            shlex.quote(name))


class Mount(Action):
    def __call__(self, mountpoint):
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
    name = 'import-envs'

    def __call__(self, *envs):
        cmds = []
        for env in envs:
            val = os.environ.get(env)
            if val is None:
                raise ArgError('No such env in host: {}'.format(env))
            cmds.append('{}={}'.format(env, shlex.quote(val)))
        return '  && '.join(cmds)


class Include(Action):

    friendly_exceptions = [
        IncludeError,
        yaml.parser.ParserError,
        yaml.scanner.ScannerError,
    ]

    def __call__(self, fname):
        # from .plash import action_list_to_layers
        with open(fname) as f:
            config = f.read()
        loaded = yaml.load(config)
        if not isinstance(loaded, list):
            raise IncludeError('yaml root element must be a list')
        actions = []
        for elem in loaded:
            if not isinstance(elem, dict):
                raise IncludeError('yaml file must be a list of dicts')
            if not len(elem) == 1:
                raise IncludeError('yaml dictionaries should contain only one element')
            sm, values = next(iter(elem.items()))
            if values is None:
                values = []
            if not isinstance(values, list):
                values = [values]
            if not sm in ACTIONS:
                raise IncludeError('No such system modifier: {}'.format(sm))
            values = [str(i) for i in values]
            actions.append((sm, values))
        return tokenize_actions(actions)

class Emerge(PackageManager):
    install = 'emerge {}'

class Layer(Action):
    def __call__(self):
        return LAYER
