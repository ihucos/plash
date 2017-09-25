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


def rand():
    return str(uuid.uuid4()).split('-')[-1]

def hashstr(stri):
    return hashlib.sha1(stri).hexdigest()

friendly_exception_sate = {'enabled': True}
def disable_friendly_exception():
    friendly_exception_sate['enabled'] = False

@contextmanager
def friendly_exception(exceptions, debug=None):
    try:
        yield
    except tuple(exceptions) as exc:
        if not friendly_exception_sate['enabled']:
            raise exc
        else:
            if debug:
                msg = 'plash error at {debug}: {message}'.format(
                    debug=debug,
                    message=str(exc))
            else:
                msg = 'plash error: {message}'.format(message=str(exc))

            print("\033[91m{}\033[0m".format(msg), file=sys.stderr)
            # raise exc # TODO: experiment with nested friendly_exception for example at load
            sys.exit(1)

class NonZeroExitStatus(Exception):
    pass

def run(command):
    p = subprocess.Popen(command)
    exit = p.wait()
    if exit != 0:
        raise NonZeroExitStatus('Exit status {exit} for: {cmd}'.format(
            cmd=' '.join(shlex.quote(i) for i in command),
            exit=exit))

# def create_executable_file(fname, script):
#     if os.path.exists(fname):
#         raise SystemExit('File {} already exists - deal with this'.format(fname))

#     with open(fname, 'w') as f:
#         f.write(script)
#     st = os.stat(fname)
#     ou.chmod(fname, st.st_mode | stat.S_IEXEC)

def get_subcommand_path(name):
    dir = os.path.abspath(os.path.dirname(sys.argv[0]))
    return os.path.abspath(os.path.join(dir, 'plash.{}'.format(name))) # what if '/' in subcommand


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
        os.setgroups([]) # for now loose supplementary groups
        os.setregid(int(gid), int(gid))
        os.setreuid(int(uid), int(uid))

def deescalate_sudo_call(func, *args, **kwargs):
    # if os.fork():
    #     return func(*args, **kwargs)
    # else:
    #     os._exit(0)
    r,w=os.pipe()
    r,w=os.fdopen(r,'r'), os.fdopen(w,'w')
    pid = os.fork()
    if pid:
        w.close()
        _, exit = os.waitpid(pid, 0)
        if exit:
            sys.exit(exit) # map what the child is doing
        data = r.readline()
        assert data
        r.close()
        return json.loads(data)
    else: # child
        deescalate_sudo()
        r.close()
        value = func(*args, **kwargs)
        serlialized = json.dumps(value)
        print(serlialized, file=w)
        w.close()
        os._exit(0)

def getargs(arglist, descr=None):
    prog = os.path.basename(sys.argv[0]).replace('.', ' ')
    parser = argparse.ArgumentParser(
        description=descr,
        prog=prog,
        # epilog=HELP)
    )
    for arg in arglist:
        if arg.endswith('*'):
            parser.add_argument(arg[:-1], nargs='*')
        else:
            parser.add_argument(arg)
    r = parser.parse_args(sys.argv[1:])
    ns = parser.parse_args(sys.argv[1:])
    for arg in arglist:
        yield getattr(ns, arg.strip('*'))

def red(stri):
    return "\033[1;31m" + stri + "\033[0;0m"

def setup_sigint_handler(): # TODO: call differently
    if not os.environ.get('PLASH_TRACEBACK', '').lower() in ('yes', '1', 'true'):
        def signal_handler(signal, frame):
            print(file=sys.stderr)
            print(red('Interrupted by user'), file=sys.stderr)
            sys.exit(130)
        signal.signal(signal.SIGINT, signal_handler)

        import traceback
        import io

        def better_ux_errno_exceptions(exctype, value, tb):
            s = io.StringIO()
            if getattr(exctype, 'errno', None):
                traceback.print_exception(exctype, value, tb, file=s, limit=-1)
                s.seek(0)
                error_msg = s.read().splitlines()[-1]
                error_msg = error_msg.split(None, 1)[-1]
                print(file=sys.stderr)
                print(red(error_msg), file=sys.stderr)
                sys.exit(1)
            else:
                sys.__excepthook__(exctype, value, traceback)
        sys.excepthook = better_ux_errno_exceptions
