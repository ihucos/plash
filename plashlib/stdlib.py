import hashlib
import os
import re
import shlex
import stat
import subprocess
import tempfile
import uuid
from base64 import b64encode
from itertools import dropwhile

from .eval import ArgError, action, eval
from .utils import hashstr

IMAGE_HINT_TEMPL = '### image hint: {}'
CLI_SHORTCUTS = [
    # shortcut, lsp, nargs
    (('-x',), [['run']], '+'),
    (('-l',), [['layer']], 0),
    (('-I',), [['include']], '+'),
    (('-i',), [['image']], 1),
    (('--alpine', '-A',), [['image', 'alpine'], ['apk']], '*'),
    (('--ubuntu', '-U',), [['image', 'ubuntu'], ['apt']], '*'),
    (('--fedora', '-F',), [['image', 'fedora'], ['dnf']], '*'),
    (('--debian', '-D',), [['image', 'debian'], ['apt']], '*'),
    (('--centos', '-C',), [['image', 'centos'], ['yum']], '*'),
    (('--arch', '-R',), [['image', 'arch'], ['pacman']], '*'),
    (('--gentoo', '-G',), [['image', 'gentoo'], ['emerge']], '*'),
]


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
def run(*args):
    'run shell script'
    return '\n'.join(args)


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
def include(*files):
    'include parameters from file'
    for file in files:
        fname = os.path.realpath(os.path.expanduser(file))
        with open(fname) as f:
            lsp = []
            tokens = dropwhile(
	        lambda l: l.startswith('#'),
	        (i[:-1] for i in f.readlines()))
            for line0, token in enumerate(tokens):
                if line0 == 0 and token.startswith('#!'):
                    continue
                if token.startswith('--'):
                    lsp.append([token[2:]])
                elif token:
                    lsp[-1].append(token)
        yield eval(lsp)


def hash_paths(paths):
    collect_files = []
    for path in paths:
        if image.path.isdir(path):
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
def rebuild_when_changed(*paths):
    'invalidate cache if path changes'
    hash = hash_paths(paths)
    return ":rebuild-when-changed hash: {}".format(hash)


@action(escape=False)
def define_package_manager(name, *lines):
    'define a new package manager'
    @action(name)
    def package_manager(*packages):
        if not packages:
            return
        sh_packages = ' '.join(pkg for pkg in packages)
        expanded_lines = [line.format(sh_packages) for line in lines]
        return eval([['run'] + expanded_lines])
    package_manager.__doc__ = "install packages via '{}'".format(name)


@action()
def pkg(*packages):
    'call the default package manager'
    raise ArgError('you need to ":set-pkg <package-manager>" to use pkg')


@action(escape=False)
def set_pkg(pm):
    'set the default package manager'
    @action('pkg')
    def pkg(*packages):
        return eval([[pm] + list(packages)])


@action(escape=False)
def all(command, *args):
    'use first argument as action, map it to other arguments'
    return eval([[command, arg] for arg in args])


@action()
def comment(*args):
    'do nothing'
    pass


@action('image', escape=False)
def image(os):
    'set the base image'
    return IMAGE_HINT_TEMPL.format(os)


@action(escape=False)
def chdir(path):
    'change directory at host'
    os.chdir(path)


@action()
def namespace(ns):
    'start a new build namespace'
    return eval([
         ['layer'],
         ['run', ': new namespace {}'.format(ns)],
         ['layer']])


eval([[
    'define-package-manager',
    'apt',
    'apt-get update',
    'apt-get install -y {}',
], [
    'define-package-manager',
    'add-apt-repository',
    'apt-get install software-properties-common',
    'run add-apt-repository -y {}',
], [
    'define-package-manager',
    'apk',
    'apk update',
    'apk add {}',
], [
    'define-package-manager',
    'yum',
    'yum install -y {}',
], [
    'define-package-manager',
    'dnf',
    'dnf install -y {}',
], [
    'define-package-manager',
    'pip',
    'pip install {}',
], [
    'define-package-manager',
    'npm',
    'npm install -g {}',
], [
    'define-package-manager',
    'pacman',
    'pacman -Sy --noconfirm {}',
], [
    'define-package-manager',
    'emerge',
    'emerge {}',
]])
