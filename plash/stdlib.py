import hashlib
import os
import re
import shlex
import stat
import subprocess
import tempfile
import uuid
from base64 import b64encode

from . import state
from .eval import ArgError, action, eval
from .utils import hashstr


@action()
def layer(command=None, *args):
    if not command:
        return eval([['original-layer']])  # fall back to buildin layer action
    else:
        lst = [['layer']]
        for arg in args:
            lst.append([command, arg])
            lst.append(['layer'])
        return eval(lst)


@action(keep_comments=True)
@action()
def run(*args):
    return '\n'.join(args)


@action()
def bust_cache():
    return ': bust cache with {}'.format(uuid.uuid4())

@action(keep_comments=True)
def write_script(fname, *lines):
    lines = list(lines)
    lsp = [['run', 'touch '+fname]]
    for line in lines:
        lsp.append(['run', "echo {} >> {}".format(shlex.quote(line), shlex.quote(fname))])
    lsp.append(['run', 'chmod 755 ' + fname])
    return eval(lsp)

@action()
def exec(binary):
    return 'mkdir -p /etc/runp && ln -fs {} /etc/runp/exec'.format(shlex.quote(binary))


@action()
def include(*files):
    for file in files:
        fname = os.path.realpath(os.path.expanduser(file))
        with open(fname) as f:
            lsp = []
            tokens = (i[:-1] for i in f.readlines())
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


@action()
def rebuild_when_changed(*paths):
    hash = hash_paths(paths)
    return ":rebuild-when-changed hash: {}".format(hash)


@action()
def define_package_manager(name, *lines):
    @action(name)
    def package_manager(*packages):
        sh_packages = ' '.join(shlex.quote(pkg) for pkg in packages)
        expanded_lines = [line.format(sh_packages) for line in lines]
        return eval([['run'] + expanded_lines])


@action()
def pkg(*packages):
    raise ArgError('you need to ":set-pkg <package-manager>" to use pkg')


@action()
def set_pkg(pm):
    @action('pkg')
    def pkg(*packages):
        return eval([[pm] + list(packages)])


@action()
def all(command, *args):
    return eval([[command, arg] for arg in args])


@action()
def comment(*args):
    pass


@action('os')
def os_(os):
    state.set_os(os)

@action()
def chdir(path):
    os.chdir(path)

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
    'emerge',
    'emerge {}',
]])
