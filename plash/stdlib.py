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
def pdb():
    import pdb
    pdb.set_trace()

@action(echo=False)
def layer(command=None, *args):
    if not command:
        return eval([['original-layer']]) # fall back to buildin layer action
    else:
        lst = [['layer']]
        for arg in args:
            lst.append([command, arg])
            lst.append(['layer'])
        return eval(lst)

@action()
def inline(*args):
    return '\n'.join(args)

templ_re = re.compile('{{\s*([^\s]*)\s*}}')
@action()
def run(*lines):
    for line in lines:
        exps = templ_re.findall(line)
        for e in exps:

            # expand exp
            if e.startswith('$'):
                host_env = os.environ.get(e[1:], '')
                expanded = shlex.quote(host_env)
            elif e.startswith('./'):
                abs_path = os.path.abspath(e)
                if not os.path.exists(abs_path):
                    raise ArgError('Path {} does not exist'.format(e))

                hash = hash_paths([abs_path])
                mount_to = os.path.join('/mnt', hash)
                state.add_mount(abs_path, mount_to)
                expanded =  shlex.quote(mount_to)
            elif e == 'pwd':
                expanded = os.getcwd()
            else:
                raise ArgError('Template var must start with a "./" or "$" or be "pwd" (got "{}")'.format('exp'))

            line = templ_re.sub(expanded, line, count=1)
        yield line

@action(echo=False)
def chdir(path):
    os.chdir(path)

def hash_paths(paths):
    collect_files = []
    for path in paths:
        if os.path.isdir(path):
            collect_files.extend(all_files(path))
        else:
            collect_files.append(path)

    hasher = hashlib.sha1()
    for fname in sorted(collect_files):
        perm = str(oct(stat.S_IMODE(os.lstat(fname).st_mode))
                  ).encode()
        with open(fname, 'rb') as f:
            fread = f.read() # well, no buffering?
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
    return "echo 'rebuild-when-changed: hash {}'".format(hash)


@action()
def define_package_manager(name, *lines):

    @action(name)
    def package_manager(*packages):
        sh_packages = ' '.join(shlex.quote(pkg) for pkg in packages)
        expanded_lines = [line.format(sh_packages) for line in lines]
        return eval([['run'] + expanded_lines])


@action()
def interactive(name=''):
    return "echo 'Exit shell when ready' && bash && : modifier name is {}".format(
        shlex.quote(name))


@action()
def mount(from_, to):
    state.add_mount(from_, to)


@action()
def bustcache():
    return  ': bust cache with {}'.format(uuid.uuid4()) 


@action(echo=False)
def pkg(*packages):
    raise ArgError('you need to ":set-pkg <package-manager>" to use pkg')

@action(echo=False)
def set_pkg(pm):
    @action('pkg', echo=False)
    def pkg(*packages):
        return eval([[pm] + list(packages)])


@action()
def with_file(command, *lines):
    file_content = '\n'.join(lines)
    encoded = b64encode(file_content.encode())
    inline_file = '<(echo {} | base64 --decode)'.format(encoded.decode())
    return eval([[command, inline_file]])


@action(echo=False)
def each_line(*args):
    if not len(args) <= 2:
        raise ArgError('needs at leat two arguments')
    args = list(args)
    file = args.pop(-1)
    with open(file) as f:
        lines = f.readlines()
    lsp = []
    for line in lines:
        lsp.append(args + [line])
    return eval(lsp)



@action(echo=False)
def all(command, *args):
    return eval([[command, arg] for arg in args])

@action('#', echo=False)
def comment(*args):
    pass

@action(echo=False)
def define(action_name, *lines):

    if not lines[0][:2] == '#!': # looks like a shebang
        lines = ['#!/usr/bin/env bash'] + list(lines)

    @action(action_name, echo=True)
    def myaction(*args):
        encoded = b64encode('\n'.join(lines).encode())
        inline_file = '<(echo {} | base64 --decode)'.format(encoded.decode())

        return ("define_tmpfile=$(mktemp /tmp/XXXXXX-{action}) && "
                "cp {inline_file} $define_tmpfile && "
                "chmod u+x $define_tmpfile && "
                '.$define_tmpfile ' + ' '.join(shlex.quote(i) for i in args) 
                ).format(inline_file=inline_file, action=action_name)


@action(echo=False)
def script(*lines):
    eval([['define', 'last-script'] + list(lines)])
    return eval([['last-script']])

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


def script2lsp(script):
    # FIXME: #: mybla: arg does not work, mybla should be ignored by comment
    lines = script.splitlines()

    # ignore shebangs
    if lines and lines[0].startswith('#!'):
        lines.pop(0)

    first_line = next(iter(l for l in lines if l))
    if not first_line.split()[0].endswith(':'):
        raise ArgError(
            'first line ("{}") must be a function and not an argument'.format(first_line))

    # tokenize
    tokens = []
    for c, line in enumerate(lines):
        if not line:
            continue
        if line.endswith((' ', '\t')):
            raise ArgError('line {} has trailing whitespace(s)'.format(c+1))
        if not line.split()[0].endswith(':'):
            tokens.append(line)
        else:
            line_tokens = line.split(' ')
            tokens.extend(line_tokens)

    # generate lsp out of the tokens
    lsp = []
    for token in tokens:
        if token.endswith(':'):
            lsp.append([token[:-1]])
        else:
            lsp[-1].append(token)

    return lsp


@action(echo=False)
def include(*files):
    for file in files:
        fname = os.path.realpath(os.path.expanduser(file))
        lsp = []
        with open(fname) as f:
            lsp = script2lsp(f.read())
        dir = os.path.dirname(fname)
        os.chdir(dir)
        yield eval(lsp)

@action('os', echo=False)
def os_(os):
    state.set_os(os)

@action(echo=False)
def entrypoint(binary):
    return '[[ -x {0} ]] && printf \'#!/bin/sh\\nexec {0} "$@"\' > /entrypoint && chmod 755 /entrypoint'.format(binary)
    # return "ln -s {} /entrypoint".format(shlex.quote(binary)) # lik doesnt work with busybox

@action(echo=False)
def host(*lines):
    lines = list(lines)
    if not lines:
        raise ArgError('needs at least one arg')
    if not lines[0].startswith('#!'):
        lines = ['#!/bin/sh'] + lines
    tmp = tempfile.mktemp()
    with open(tmp, 'w') as f:
        f.write('\n'.join(lines))
    os.chmod(tmp, 0o755) # check if this access rights are right
    subprocess.check_call([tmp])

@action(echo=False)
def device(name, path):
    return eval([['run', 'mkdir -p /etc/runp && echo {} >> /etc/runp/devices'.format(shlex.quote(name + ' ' + path))]])

eval(script2lsp('''

define-package-manager: apt
apt-get update
apt-get install -y {}

define-package-manager: add-apt-repository
apt-get install software-properties-common
run add-apt-repository -y {}

define-package-manager: apk
apk update
apk add {}

define-package-manager: yum
yum install -y {}

define-package-manager: dnf
dnf install -y {}

define-package-manager: pip
pip install {}

define-package-manager: npm
npm install -g {}

define-package-manager: emerge
emerge {}

'''))
