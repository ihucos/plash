import hashlib
import os
import shlex
import stat
import subprocess
import sys
import uuid
from base64 import b64encode

from . import state
from .eval import action, eval, ArgError
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
def run(*args):
    return '\n'.join(args)

@action(echo=False)
def silentrun(*args):
    return ' '.join(args)

@action(echo=False)
def chdir(path):
    os.chdir(path)

@action()
def warp(command, *args):
    init = []
    cleanup = []
    replaced_args = []
    for arg in args:
        if arg.startswith('{') and arg.endswith('}'):
            path = arg[1:-1]
            if os.path.isabs(path):
                raise ArgError('path should be relative: {}'.format(path))
            if not os.path.exists(path):
                raise ArgError('Path {} does not exist'.format(path))
            p = subprocess.Popen(['tar', '-c', path],
                                 stderr=subprocess.DEVNULL, stdout=subprocess.PIPE)
            assert p.wait() == 0
            out = p.stdout.read()
            baseout = b64encode(out).decode()
            outpath = os.path.join('/tmp', hashstr(out)[:8])
            init.append('mkdir {}'.format(outpath))
            init.append('echo {baseout} | base64 --decode | tar -C {out} -x{extra}'.format(
                out=outpath, baseout=baseout, extra=' --strip-components 1' if os.path.isdir(path) else ''))
            replaced_args.append(os.path.join(outpath, path))
            # replaced_args.append(outpath)
        else:
            replaced_args.append(arg)

    return eval([
        ['silentrun', ' && '.join(init)],
        [command] + replaced_args,
        ['silentrun', ' && '.join(cleanup)],
    ])
    # return '{}\n{}\n'.format(
    #     ' && '.join(init), ' '.join(replaced_args), ' && '.join(cleanup))

# class Home(Action):

#     def __call__(self):
#         return self.eval([['include', path.join(home_path, '.plash.yaml')]])


# print(eval([['layer-each', 'inline', 'hi', 'ho']]))


class RebuildWhenChanged():

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
            hasher.update(hashstr(fread))

        hash = hasher.hexdigest()
        return "echo 'rebuild-when-changed: hash {}'".format(hash)

    def _extract_files(self, dir):
        for (dirpath, dirnames, filenames) in os.walk(dir):
            for filename in filenames:
                fname = os.sep.join([dirpath, filename])
                yield fname

action('rebuild-when-changed')(RebuildWhenChanged())

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
def mount(*mountpoints):
    for mountpoint in mountpoints:
        mountpoint = os.path.realpath(mountpoint)
        if not os.path.isdir(mountpoint):
            raise ArgError('{} is not a directory'.format(mountpoint))
        from_ = os.path.join('/.host_fs_do_not_use', mountpoint.lstrip('/'))
        cmd = "mkdir -p {dst} && mount --bind {src} {dst}".format(
            src=shlex.quote(from_),
            dst=shlex.quote(mountpoint))
        yield cmd

@action()
def import_env(*envs):
    for env in envs:
        parts = env.split(':')
        if len(parts) == 1:
            host_env = env
            guest_env = env
        elif len(parts) == 2:
            host_env, guest_env = parts
        else:
            raise ArgError('env can only contain one ":"')
        host_val = os.environ.get(host_env)
        if host_val is None:
            return
            # raise ArgError('No such env in host: {}'.format(host_env))
        yield 'export {}={}'.format(guest_env, shlex.quote(host_val))


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
        fname = os.path.realpath(file)
        lsp = []
        with open(fname) as f:
            lsp = script2lsp(f.read())
        yield eval(lsp)

@action('os', echo=False)
def os_(os):
    state.set_os(os)

@action(echo=False)
def cmd(os):
    state.set_base_command(os)


eval(script2lsp('''

define-package-manager: apt
apt-get update
apt-get install -y {}

define-package-manager: add-apt-repository
apt software-properties-common
run add-apt-repository -y {}

define-package-manager: apk
apk update
apk add  {}

define-package-manager: yum
yum install -y {}

define-package-manager: pip
pip install {}

define-package-manager: npm
npm install -g {}

define-package-manager: emerge
emerge {}

'''))
