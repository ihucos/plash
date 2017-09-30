#!/usr/bin/env python3
import os
import subprocess
import sys

def filter_positionals(args):
    positional = []
    filtered_args = []
    found_first_opt = False
    while args:
        arg = args.pop(0)
        if not arg.startswith('-') and not found_first_opt:
            positional.append(arg)
        elif arg == '--':
            positional += args
            args = None
        else:
            filtered_args.append(arg)
            found_first_opt = True
    return positional, filtered_args

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
