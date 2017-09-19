import hashlib
import os
import shlex
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

            print("\033[91m{}\033[0m".format(msg))
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
    # very ugly, no idea how this is suppsed to be done
    module_dir = os.path.dirname(os.path.abspath(__file__))
    subcommands_dir = os.path.join(module_dir, '..', 'data', 'bin')
    return os.path.abspath(os.path.join(subcommands_dir, name)) # what if '/' in subcommand


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
