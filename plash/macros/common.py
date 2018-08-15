import hashlib
import os
import shlex
import stat
import subprocess
import uuid

from plash.eval import (eval, hint, join_result, register_macro,
                        shell_escape_args)
from plash.utils import hashstr


@register_macro()
def layer(command=None, *args):
    'start a new layer'
    if not command:
        return eval([['hint', 'layer']])  # fall back to buildin layer macro
    else:
        lst = [['layer']]
        for arg in args:
            lst.append([command, arg])
            lst.append(['layer'])
        return eval(lst)


@register_macro()
def run(*cmds):
    'run shell script'
    return '\n'.join(cmds)


@register_macro()
@shell_escape_args
@join_result
def import_envs(*envs):
    'import environment variables from host'
    for env in envs:
        parts = env.split(':', 1)
        if len(parts) == 1:
            export_as = env
        else:
            env, export_as = parts
        yield '{}={}'.format(export_as, shlex.quote(os.environ[env]))


@register_macro()
def invalidate_cache():
    'Invalidate cache'
    return ': bust cache with {}'.format(uuid.uuid4())


@register_macro()
@shell_escape_args
@join_result
def write_file(fname, *lines):
    'write an executable file'
    yield 'touch {}'.format(fname)
    for line in lines:
        yield "echo {} >> {}".format(line, fname)


@register_macro()
@shell_escape_args
@join_result
def write_script(fname, *lines):
    yield write_file(fname, *lines)
    yield 'chmod 755 {}'.format(fname)


@register_macro()
def eval_file(file):
    'include parameters from file'

    fname = os.path.realpath(os.path.expanduser(file))
    with open(fname) as f:
        inscript = f.read()

    return subprocess.run(
        ['plash-eval'],
        input=inscript.encode(),
        check=True,
        stdout=subprocess.PIPE).stdout.decode()


@register_macro('eval')
def eval_macro(stri):
    tokens = shlex.split(stri)
    return subprocess.run(
        ['plash-eval'],
        input='\n'.join(tokens).encode(),
        check=True,
        stdout=subprocess.PIPE).stdout.decode()


class HashPaths:
    'only rebuild if anything in a path has changed'

    def _list_all_files(self, dir):
        for (dirpath, dirnames, filenames) in os.walk(dir):
            for filename in filenames:
                fname = os.sep.join([dirpath, filename])
                yield fname

    def __call__(self, paths):

        collect_files = []
        for path in paths:
            if os.path.isdir(path):
                collect_files.extend(self._list_all_files(path))
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
        return ": hash: {}".format(hash)


register_macro('hash-path')(HashPaths())


@register_macro('#')
def comment(*args):
    'do nothing'


@register_macro('image')
def image(os):
    'set the base image'
    return hint('image', os)


@register_macro('exec')
def exec(exec_path):
    'hint what to run in this container'
    return hint('exec', exec_path)


@register_macro()
def namespace(ns):
    'start a new build namespace'
    return eval([['layer'], ['run', ': new namespace {}'.format(ns)],
                 ['layer']])
