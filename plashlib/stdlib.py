import hashlib
import os
import shlex
import stat
import sys
import uuid
from itertools import dropwhile
import subprocess

from .eval import action, eval, get_actions
from .utils import hashstr

IMAGE_HINT_TEMPL = '### image hint: {}'


@action(escape=False)
def layer(command=None, *args):
    'start a new layer'
    if not command:
        return eval([['original-layer']])  # fall back to buildin layer action
    else:
        lst = [['layer']]
        for arg in args:
            lst.append([command, arg])
            lst.append(['layer'])
        return eval(lst)


@action(escape=False, keep_comments=True)
def run(*cmds):
    'run shell script'
    return '\n'.join(cmds)


@action()
def align_cwd():
    'cd tho the pwd at the host'
    yield 'cd {}'.format(shlex.quote(os.getcwd()))


@action()
def import_envs(*envs):
    'import environment variables from host'
    for env in envs:
        parts = env.split(':', 1)
        if len(parts) == 1:
            export_as = env
        else:
            env, export_as = parts
        yield '{}={}'.format(export_as, shlex.quote(os.environ[env]))


@action()
def bust_cache():
    'Invalidate cache'
    return ': bust cache with {}'.format(uuid.uuid4())


@action(keep_comments=True)
def write_script(fname, *lines):
    'write an executable file'
    yield 'touch {}'.format(fname)
    for line in lines:
        yield "echo {} >> {}".format(line, fname)
    yield 'chmod 755 {}'.format(fname)


@action(escape=False)
def include(file):
    'include parameters from file'

    fname = os.path.realpath(os.path.expanduser(file))
    with open(fname) as f:

        # little magic: remove the first lines starting with comments
        inscript = ''.join(
            dropwhile(lambda l: l.startswith('#'), (i for i in f.readlines())))

    return subprocess.run(
        ['plash-eval'],
        input=''.join(inscript).encode(),
        check=True,
        stdout=subprocess.PIPE).stdout.decode()


@action(escape=False)
def include_string(stri):
    tokens = shlex.split(stri)
    return subprocess.run(
        ['plash-eval'],
        input='\n'.join(tokens).encode(),
        check=True,
        stdout=subprocess.PIPE).stdout.decode()


def hash_paths(paths):
    collect_files = []
    for path in paths:
        if os.path.isdir(path):
            collect_files.extend(all_files(path))
        else:
            collect_files.append(path)

    hasher = hashlib.sha1()
    for fname in sorted(collect_files):
        perm = str(oct(stat.S_IMODE(os.lstat(fname).st_mode))).encode()
        with open(fname, 'rb') as f:
            fread = f.read()  # well, no buffering?
        hasher.update(fname.encode())
        hasher.update(perm)

        hasher.update(hashstr(fread).encode())

    hash = hasher.hexdigest()
    return hash


def all_files(dir):
    for (dirpath, dirnames, filenames) in os.walk(dir):
        for filename in filenames:
            fname = os.sep.join([dirpath, filename])
            yield fname


@action(escape=False)
def watch(*paths):
    'only rebuild if anything in a path has changed'
    hash = hash_paths(paths)
    return ": watch hash: {}".format(hash)


@action(escape=False, group='package managers')
def defpm(name, *lines):
    'define a new package manager'

    @action(name, group='package managers')
    def package_manager(*packages):
        if not packages:
            return
        sh_packages = ' '.join(pkg for pkg in packages)
        expanded_lines = [line.format(sh_packages) for line in lines]
        return eval([['run'] + expanded_lines])

    package_manager.__doc__ = "install packages with {}".format(name)


@action(escape=False)
def map(command, *args):
    'use first argument as action, map it to other arguments'
    return eval([[command, arg] for arg in args])


@action()
def comment(*args):
    'do nothing'


@action('image', escape=False)
def image(os):
    'set the base image'
    return IMAGE_HINT_TEMPL.format(os)


@action()
def namespace(ns):
    'start a new build namespace'
    return eval([['layer'], ['run', ': new namespace {}'.format(ns)],
                 ['layer']])


@action('f')
@action()
def workaround_unionfs():
    'workaround apt so it works despite this issue: https://github.com/rpodgorny/unionfs-fuse/issues/78'
    script = '''if [ -d /etc/apt/apt.conf.d ]; then
echo 'Dir::Log::Terminal "/dev/null";' > /etc/apt/apt.conf.d/unionfs_workaround
echo 'APT::Sandbox::User "root";' >> /etc/apt/apt.conf.d/unionfs_workarounds
: See https://github.com/rpodgorny/unionfs-fuse/issues/78
chown root:root /var/cache/apt/archives/partial || true
fi'''
    return eval([['run', script]])


@action()
def list():
    'list all actions'
    actions = get_actions()
    prev_group = None
    for name, func in sorted(
            actions.items(), key=lambda i: (i[1]._plash_group or '', i[0])):
        group = func._plash_group or 'main'
        if group != prev_group:
            prev_group is None or print()
            print('    ## {} ##'.format(group), file=sys.stderr)
        print('{: <16} {}'.format(name, func.__doc__), file=sys.stderr)
        prev_group = group
    sys.exit(1)


ALIASES = dict(
    x=[['run']],
    l=[['layer']],
    I=[['include']],
    i=[['image']],
    alpine=[['image', 'alpine'], ['apk']],
    A=[['alpine']],
    ubuntu=[['image', 'ubuntu'], ['apt']],
    U=[['ubuntu']],
    fedora=[['image', 'fedora'], ['dnf']],
    F=[['fedora']],
    debian=[['image', 'debian'], ['apt']],
    D=[['debian']],
    centos=[['image', 'centos'], ['yum']],
    C=[['centos']],
    arch=[['image', 'arch'], ['pacman']],
    R=[['arch']],
    gentoo=[['image', 'gentoo'], ['emerge']],
    G=[['gentoo']],
)

for name, macro in ALIASES.items():

    def bounder(macro=macro):
        def func(*args):
            # list(args) throws exception some really funny reason
            # therefore the list comprehension
            args = [i for i in args]
            return eval(macro[:-1] + [macro[-1] + args])

        func.__doc__ = 'macro for: {}[ARG1 [ARG2 [...]]]'.format(' '.join(
            '--' + i[0] + ' ' + ' '.join(i[1:]) for i in macro))
        return func

    func = bounder()
    action(name=name, group='macros', escape=False)(func)

eval([[
    'defpm',
    'apt',
    'apt-get update',
    'apt-get install -y {}',
], [
    'defpm',
    'add-apt-repository',
    'apt-get install software-properties-common',
    'run add-apt-repository -y {}',
], [
    'defpm',
    'apk',
    'apk update',
    'apk add {}',
], [
    'defpm',
    'yum',
    'yum install -y {}',
], [
    'defpm',
    'dnf',
    'dnf install -y {}',
], [
    'defpm',
    'pip',
    'pip install {}',
], [
    'defpm',
    'pip3',
    'pip3 install {}',
], [
    'defpm',
    'npm',
    'npm install -g {}',
], [
    'defpm',
    'pacman',
    'pacman -Sy --noconfirm {}',
], [
    'defpm',
    'emerge',
    'emerge {}',
]])
