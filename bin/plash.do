#!/usr/bin/env python3
from plash.utils import setup_sigint_handler; setup_sigint_handler()

import os
import subprocess
import sys

from plash.utils import filter_positionals, get_subcommand_path

cmd, args = filter_positionals(sys.argv[1:])

build_script = get_subcommand_path('build')
def p():
      os.environ['PLASH_DO_CALLED'] = '1'
try:
      out = subprocess.check_output([build_script] + args, preexec_fn=p)
except subprocess.CalledProcessError as exc:
      sys.exit(exc.returncode)
container_id = out[:-1]
script = get_subcommand_path(cmd[0])
os.execvpe(script, [script, container_id] + cmd[1:], os.environ)
