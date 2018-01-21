# import where needed to make startup faster
import os
import sys
from contextlib import contextmanager
from os.path import join
from tempfile import mkdtemp

ERROR_COLOR = 1
INFO_COLOR = 4

PLASH_DATA = os.environ.get('PLASH_DATA', '/var/lib/plash')
TMP_DIR = join(PLASH_DATA, 'tmp')
BUILDS_DIR = join(PLASH_DATA, 'builds')
INDEX_DIR = join(PLASH_DATA, 'index')
CACHE_KEYS_DIR = join(PLASH_DATA, 'cache_keys')


def hashstr(stri):
    import hashlib
    return hashlib.sha1(stri).hexdigest()


@contextmanager
def catch_and_die(exceptions, debug=None, ignore=None):
    try:
        yield
    except tuple(exceptions) as exc:
        if ignore and isinstance(exc, ignore):
            raise
        msg = str(exc)
        if msg.startswith('<') and msg.endswith('>'):
            msg = msg[1:-1]
        if debug:
            msg = '{debug}: {message}'.format(debug=debug, message=msg)
        die(msg)


def deescalate_sudo():
    import pwd
    import grp
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
    prog = os.path.basename(sys.argv[0])
    print(color('error: {}: '.format(prog), ERROR_COLOR) + msg, file=sys.stderr)
    sys.exit(exit)


def info(msg):
    print(color(msg, INFO_COLOR), file=sys.stderr)

def die_with_usage(*, hint=False):
    prog = os.path.basename(sys.argv[0])
    printed_usage = False
    with open(sys.argv[0]) as f:
        for line in f.readlines():
            if line.startswith('# usage:'):
                usage_line = line[2:]
                print('{}: {}'.format(prog, usage_line), end='')
                printed_usage = True
    assert printed_usage, 'could not find usage comment'
    if hint:
        print('{}: usage hint: {}'.format(prog, hint), file=sys.stderr)
    sys.exit(2)

def handle_help_flag():
    if len(sys.argv) >= 2 and sys.argv[1] in ('--help', '-h'):
        with open(sys.argv[0]) as f:
            do_print = False
            for line in f.readlines():
                if line.startswith('# usage:'):
                    do_print = True
                elif line and not line.startswith('#'):
                    break
                if do_print:
                    print(line[2:], end='')
        sys.exit(0)

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

def handle_build_args():
    import subprocess
    if len(sys.argv) >= 2 and sys.argv[1].startswith('-'):
        cmd, args = filter_positionals(sys.argv[1:])
        try:
            out = subprocess.check_output(['plash-build'] + args)
        except subprocess.CalledProcessError as exc:
            die('build failed')
        container_id = out[:-1]
        os.execvpe(sys.argv[0], [sys.argv[0], container_id] + cmd, os.environ)

def nodepath_or_die(unescaped_container):
    container = unescaped_container.replace('/', '%')
    try:
        # FIXME: security check that container does not contain bad chars
        with catch_and_die([OSError], ignore=FileNotFoundError, debug='readlink'):
            nodepath = os.readlink(os.path.join(INDEX_DIR, container))
        with catch_and_die([OSError], ignore=FileNotFoundError, debug='stat'):
            os.stat(nodepath)
        return nodepath
    except FileNotFoundError:
        die('no container {}'.format(repr(unescaped_container)), exit=3)

def get_nodepath(unescaped_container):
    if not unescaped_container:
        raise ValueError('container can not be empty')
    container = unescaped_container.replace('/', '%')
    try:
        nodepath = os.readlink(os.path.join(INDEX_DIR, container))
        if os.path.exists(nodepath):
            return nodepath
        return False
    except FileNotFoundError:
        return False

def get_default_shell(passwd_file):
    with open(passwd_file) as f:
        #  the first entry is the root entry
        #  https://security.stackexchange.com/questions/96241/why-require-root-to-be-the-first-entry-in-etc-passwd
        root_entry = f.readline().rstrip('\n')
        default_root_shell = root_entry.split(":")[6]
        return default_root_shell
