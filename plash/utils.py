import argparse
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

ERROR_COLOR = 1
INFO_COLOR = 4


def hashstr(stri):
    return hashlib.sha1(stri).hexdigest()

@contextmanager
def catch_and_die(exceptions, debug=None):
    try:
        yield
    except tuple(exceptions) as exc:
        if debug:
            msg = '{debug}: {message}'.format(
                debug=debug, message=str(exc))
        else:
            msg = str(exc)
        die(msg)


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


def deescalate_sudo_call(func, *args, **kwargs):
    r, w = os.pipe()
    r, w = os.fdopen(r, 'r'), os.fdopen(w, 'w')
    pid = os.fork()
    if pid:
        w.close()
        _, exit = os.waitpid(pid, 0)
        if exit:
            exit = exit // 256
            sys.exit(exit)
        data = r.readline()
        assert data
        r.close()
        return json.loads(data)
    else:  # child
        deescalate_sudo()
        r.close()
        value = func(*args, **kwargs)
        serlialized = json.dumps(value)
        print(serlialized, file=w)
        w.close()
        os._exit(0)


def color(stri, color, isatty_fd_check=2):
    if os.isatty(isatty_fd_check):
        return "\033[38;05;{}m".format(int(color)) + stri + "\033[0;0m"
    return stri


def setup_sigint_handler():  # TODO: call differently
    # return
    # if not os.environ.get('PLASH_TRACEBACK', '').lower() in ('yes', '1', 'true'):
    #     def signal_handler(signal, frame):
    #         print(file=sys.stderr)
    #         die('Interrupted by user', exit=130)
    #     signal.signal(signal.SIGINT, signal_handler)

    #     import traceback
    #     import io

    def better_ux_errno_exceptions(exctype, value, tb):
        if exctype is KeyboardInterrupt:
            if os.environ.get(
                    'PLASH_DO_CALLED'
            ):  # so we don't see the interrupted by message two times, the plash do is already handling that
                sys.exit(130)
            print(file=sys.stderr)
            die('Interrupted by user', exit=130)
        if getattr(exctype, 'errno', None):
            s = io.StringIO()
            traceback.print_exception(exctype, value, tb, file=s, limit=-1)
            s.seek(0)
            error_msg = s.read().splitlines()[-1]
            error_msg = error_msg.split(None, 1)[-1]
            print(file=sys.stderr)
            die(error_msg)
        else:
            sys.__excepthook__(exctype, value, traceback)

    sys.excepthook = better_ux_errno_exceptions


def die(msg, exit=1):
    print(color('ERROR ', ERROR_COLOR) + msg, file=sys.stderr)
    sys.exit(exit)


def info(msg):
    print(color(msg.capitalize(), INFO_COLOR), file=sys.stderr)


def get_container_or_die(cid):
    from .core import Container, ContainerDoesNotExist
    with catch_and_die([ContainerDoesNotExist]):
        return Container.by_alias(cid)
