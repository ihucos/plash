#!/usr/bin/env python3
import os
import subprocess
import sys

from plash.utils import filter_positionals


def get_subcommand_path(name):
    dir = os.path.abspath(os.path.dirname(sys.argv[0]))
    return os.path.abspath(os.path.join(
        dir, 'plash.{}'.format(name)))  # what if '/' in subcommand


cmd, args = filter_positionals(sys.argv[1:])

build_script = get_subcommand_path('build')

try:
    out = subprocess.check_output(
        [build_script] + args,
        preexec_fn=lambda: os.putenv('PLASH_DO_CALLED', '1'))
except subprocess.CalledProcessError as exc:
    sys.exit(exc.returncode)
container_id = out[:-1]
script = get_subcommand_path(cmd[0])
os.execvpe(script, [script, container_id] + cmd[1:], os.environ)
