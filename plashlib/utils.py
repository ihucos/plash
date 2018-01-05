import argparse
import base64
import grp
import hashlib
import json
import os
import pwd
import shlex
import signal
import stat
import subprocess
import sys
import uuid
from contextlib import contextmanager
from os.path import join
from subprocess import CalledProcessError, check_output

ERROR_COLOR = 1
INFO_COLOR = 4

BASE_DIR = os.environ.get('PLASH_DATA', '/var/lib/plash')
TMP_DIR = join(BASE_DIR, 'tmp')
BUILDS_DIR = join(BASE_DIR, 'builds')
LINKS_DIR = join(BASE_DIR, 'links')


def hashstr(stri):
    return hashlib.sha1(stri).hexdigest()


@contextmanager
def catch_and_die(exceptions, debug=None, ignore=None):
    try:
        yield
    except tuple(exceptions) as exc:
        if ignore and isinstance(exc, ignore):
            raise
        program = os.path.basename(sys.argv[0])
        msg = str(exc)
        if msg.startswith('<') and msg.endswith('>'):
            msg = msg[1:-1]
        if debug:
            msg = '{debug}: {message}'.format(debug=debug, message=msg)
        die('at ' + program + ': ' + msg)


def deescalate_sudo():
    uid = os.environ.get('SUDO_UID')
    gid = os.environ.get('SUDO_GID')
    if uid and gid:
        uid = int(uid)
        gid = int(gid)
        # username = pwd.getpwuid(uid).pw_name
        # groups = [g.gr_gid for g in grp.getgrall() if username in g.gr_mem]
        os.setgroups([])  # for now loose supplementary groups
        os.setregid(int(gid), int(gid))
        os.setreuid(int(uid), int(uid))


def color(stri, color, isatty_fd_check=2):
    if os.isatty(isatty_fd_check):
        return "\033[38;05;{}m".format(int(color)) + stri + "\033[0;0m"
    return stri

def die(msg, exit=1):
    print(color('plash: error: ', ERROR_COLOR) + msg, file=sys.stderr)
    sys.exit(exit)


def info(msg):
    print(color(msg, INFO_COLOR), file=sys.stderr)

def call_plash_nodepath(container):
    try:
        return check_output(['plash-nodepath',
                                  container]).decode().strip('\n')
    except CalledProcessError as exc:
        if exc.returncode == 3:
            sys.exit(exc.returncode)
        with catch_and_die([CalledProcessError]):
            raise

def _get_subcommand():
    return sys.argv[0].split('.')[-1]

def handle_help_flag():
    subcommand = _get_subcommand()
    if sys.argv[1:2] == ['--help']:
        os.execvp('plash-help', ['plash-help', subcommand])

def die_with_usage():
    subcommand = _get_subcommand()
    subprocess.check_call(['plash-help', '--usage', subcommand])
    sys.exit(2)

