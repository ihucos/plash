#!/usr/bin/env python3
# vim: set filetype=python:
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


cmd, args = filter_positionals(sys.argv[1:])
if not cmd:
    sys.stderr.write('<build args> plash-command [arg [arg [arg...]]]\n')
    sys.exit(2)

try:
    out = subprocess.check_output(['plash.build'] + args)
except subprocess.CalledProcessError as exc:
    sys.exit(exc.returncode)
container_id = out[:-1]
script = 'plash.{}'.format(cmd[0])  # well, SECURITY CHECK NEEDED)
os.execvpe(script, [script, container_id] + cmd[1:], os.environ)
