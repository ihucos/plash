import argparse
import hashlib
import logging
import os
import platform
import shlex
import stat
import subprocess
import sys
import uuid
from base64 import b64encode
from collections import OrderedDict, namedtuple
from contextlib import contextmanager
from os import environ, path
from os.path import expanduser
from urllib.parse import urlparse

import yaml

from .actions.base import actions as ACTIONS
from .actions.core import LAYER
from .distros.base import distros as DISTROS
from .utils import friendly_exception
from .virt import BuildError, LayeredDockerBuildable

home_directory = expanduser("~")
script_dir = os.path.dirname(os.path.realpath(__file__))


class RunImage(LayeredDockerBuildable):

    def __init__(self, os_obj, cmds):
        self.os_obj = os_obj
        self.cmds = cmds

    @property
    def short_name(self):
        return self.name[0]

    @property
    def name(self):
        return self.__class__.__name__.lower()

    def get_base_image_name(self):
        return self.os_obj.get_image_name()

    def get_build_commands(self):
        return self.cmds

    def build_all(self, quiet=False):
        self.os_obj.build(quiet)
        self.build(quiet)

    def ensure_builded_all(self, quiet=False):
        self.os_obj.ensure_builded(quiet)
        self.ensure_builded(quiet)

    def run(self, cmd_with_args, extra_envs={}):

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
            # '-v', '/var/run/docker.sock:/var/run/docker.sock',
            '-v', '/tmp:/tmp',
            '-v', '{}:{}'.format(home_directory, home_directory),
            '--rm',
            self.get_image_name(),
        ] + list(cmd_with_args)

        for env, env_val in dict(environ, **extra_envs).items():
            if env not in ['PATH', 'LC_ALL', 'LANG']: # blacklist more envs
            # if env not in ['PATH']: # blacklist more envs
                args.insert(2, '-e')
                args.insert(3, '{}={}'.format(env, shlex.quote(env_val)))  # SECURITY: is shlex.quote safe?

        return subprocess.Popen(args).wait()


class OrderAction(argparse.Action):
    def __call__(self, parser, namespace, values, option_string=None):
        if not 'ordered_args' in namespace:
            setattr(namespace, 'ordered_args', [])
        previous = namespace.ordered_args
        previous.append((self.dest.replace('_', '-'), values))
        setattr(namespace, 'ordered_args', previous)

def main():
    HELP = 'my help'
    PROG = 'plash'
    parser = argparse.ArgumentParser(
        formatter_class=argparse.RawTextHelpFormatter,
        description='Run programm from any Linux',
        prog=PROG,
        epilog=HELP)


    parser.add_argument("--quiet", action='store_true')
    parser.add_argument("--noop", action='store_true')

    parser.add_argument(
        "exec", type=str, nargs='*', default=['bash'], help='What to execute in container')

    for pm in ACTIONS.values():

        args = ["--{}".format(pm.name)]
        short_name = getattr(pm, 'short_name', None)
        if short_name:
            args.insert(0, "-{}".format(pm.short_name))

        parser.add_argument(*args, type=str, nargs="*", help='install with {}'.format(pm.name), action=OrderAction)

    
    for os in sorted(DISTROS, key=lambda o: o.name):
        parser.add_argument('-{}'.format(os.short_name), '--{}'.format(os.name), dest='os', action='append_const', const=os)

    parser.add_argument("--rebuild", default=False, action='store_true')
    parser.add_argument("--install", default=False)


    prog_args = sys.argv[1:]
    if len(prog_args) >= 1 and not prog_args[0].startswith('-'):
        prog_args = ['--ubuntu', '--apt-from-command', prog_args[0], '--'] + prog_args
    args = parser.parse_args(prog_args)

    if not args.os:
        parser.error('specify at least on operating system')
    elif len(args.os) > 1:
        parser.error('specify only one operating system')

    # if args.install is False:
    #     install = False
    # elif not len(args.install):
    #     # default value
    #     install = '/usr/local/bin/{}'.format(args.exec[0])
    # elif len(args.install) == 1:
    #     install = args.install[0]
    # else:
    #     parser.error('--install needs one or no argument')

    instrs = getattr(args, 'ordered_args', [])
    from .actions.core import tokenize_actions # FIXME: take that from somewhere else
    tokens =  tokenize_actions(instrs)

    # split tokens by LAYER
    layers = [[]]
    while tokens:
        elem = tokens.pop(0)
        if not elem is LAYER:
            layers[-1].append(elem)
        else:
            layers.append([])
    
    command_layers = [' && '.join(layer) for layer in layers]
    pi = RunImage(args.os[0], command_layers)

    with friendly_exception([BuildError]):
        if not args.rebuild:
            pi.ensure_builded_all(quiet=args.quiet)
        else:
            pi.build_all(quiet=args.quiet)


    if not args.install:
        if not args.noop:
            exit = pi.run(args.exec, extra_envs={
                'PLASH_ENV': args.os[0].name,
            })
            sys.exit(exit)

    else:
        install_to = args.install
        argv = sys.argv[1:]
        install_index =  argv.index('--install')
        argv.pop(install_index)
        argv.pop(install_index)
        run_script = '#!/bin/sh\nplash {} "$@"\n'.format(' '.join(argv))
        with friendly_exception([IOError], 'install'):
            create_executable_file(install_to, run_script)
        print('Installed to {}'.format(install_to))
