import hashlib
import os
import shlex
import stat
import subprocess
import uuid

from plash.eval import (eval, hint, join_result, register_macro,
                        shell_escape_args)
from plash.utils import hashstr, run_write_read


@register_macro()
def layer(command=None, *args):
    'hint the start of a new layer'
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
    'directly emit shell script'
    return '\n'.join(cmds)


@register_macro()
@shell_escape_args
@join_result
def import_env(*envs):
    'import environment variables from host'
    for env in envs:
        parts = env.split(':', 1)
        if len(parts) == 1:
            export_as = env
        else:
            env, export_as = parts
        yield '{}={}'.format(export_as, shlex.quote(os.environ[env]))


@register_macro()
def invalidate_layer():
    'invalidate the cache of the current layer'
    return ': invalidate cache with {}'.format(uuid.uuid4())


@register_macro()
@shell_escape_args
@join_result
def write_file(fname, *lines):
    'write lines to a file'
    yield 'touch {}'.format(fname)
    for line in lines:
        yield "echo {} >> {}".format(line, fname)


@register_macro()
@join_result
def write_script(fname, *lines):
    'write an executable (755) file to the filesystem'
    yield write_file(fname, *lines)
    yield 'chmod 755 {}'.format(fname)


@register_macro()
def eval_file(file):
    'evaluate file content as expressions'

    fname = os.path.realpath(os.path.expanduser(file))
    with open(fname) as f:
        inscript = f.read()

    return run_write_read(
        ['plash-eval'], inscript.encode()
        ).stdout.decode()


@register_macro('eval')
def eval_macro(stri):
    'evaluate expressions passed as string'
    tokens = shlex.split(stri)

    return run_write_read(
        ['plash-eval'], '\n'.join(tokens).encode()
        ).stdout.decode()

class HashPaths:
    'recursively hash files and add as cache key'

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


@register_macro('from')
def from_(os):
    'hint the base image'
    return hint('image', os)


@register_macro()
def entrypoint(exec_path):
    'hint default command for this build'
    return hint('exec', exec_path)

@register_macro()
def entrypoint_script(*lines):
    'write lines to /entrypoint and hint it as default command'
    lines = list(lines)
    if lines and not lines[0].startswith('#!'):
        lines.insert(0, '#!/bin/sh')
    return eval([['entrypoint', '/entrypoint'], ['write-script', '/entrypoint'] + lines])


@register_macro()
def namespace(ns):
    'start a new build namespace'
    return eval([['layer'], ['run', ': new namespace {}'.format(ns)],
                 ['layer']])
