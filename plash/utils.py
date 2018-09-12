# import where needed to make startup faster
import os
import sys
from contextlib import contextmanager
from os.path import join

ERROR_COLOR = 1
INFO_COLOR = 4


def hashstr(stri):
    import hashlib
    return hashlib.sha1(stri).hexdigest()


@contextmanager
def catch_and_die(exceptions,
                  debug=None,
                  debug_class=False,
                  ignore=None,
                  silent=False,
                  exit=1):
    try:
        yield
    except tuple(exceptions) as exc:
        if ignore and isinstance(exc, ignore):
            raise
        if silent:
            sys.exit(exit)
        msg = str(exc)
        if msg.startswith('<') and msg.endswith('>'):
            msg = msg[1:-1]
        if debug_class:
            debug = exc.__class__.__name__
        if debug:
            msg = '{debug}: {message}'.format(debug=debug, message=msg)
        die(msg, exit=exit)


def get_plash_data():
    if os.getuid():
        default = '~/.plashdata'
    else:
        default = '/var/lib/plash'
    dir = os.environ.get('PLASH_DATA', default)
    return os.path.expanduser(dir)


def color(stri, color, isatty_fd_check=2):
    if os.environ.get('TERM') != 'dumb' and os.isatty(isatty_fd_check):
        return "\033[38;05;{}m".format(int(color)) + stri + "\033[0;0m"
    return stri


def die(msg, exit=1):
    prog = os.path.basename(sys.argv[0])
    print(
        '{} {}'.format(color('plash error:', ERROR_COLOR), msg),
        file=sys.stderr)
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
        with catch_and_die([subprocess.CalledProcessError], silent=True):
            out = subprocess.check_output(['plash-build'] + args)
        container_id = out[:-1]
        os.execlp(sys.argv[0], sys.argv[0], container_id, *cmd)


def nodepath_or_die(container, allow_root_container=False):
    import subprocess

    extra = [] if not allow_root_container else ['--allow-root-container']
    with catch_and_die([subprocess.CalledProcessError], silent=True):
        return subprocess.check_output(
            ['plash-nodepath', str(container)] + extra, ).decode().strip('\n')


def get_default_shell(passwd_file):
    with open(passwd_file) as f:
        #  the first entry is the root entry
        #  https://security.stackexchange.com/questions/96241/why-require-root-to-be-the-first-entry-in-etc-passwd
        root_entry = f.readline().rstrip('\n')
        default_root_shell = root_entry.split(":")[6]
        return default_root_shell


def plash_map(*args):
    from subprocess import check_output
    'thin wrapper around plash map'
    out = check_output(['plash-map'] + list(args))
    if out == '':
        return None
    return out.decode().strip('\n')


def assert_initialized():
    last_inited = join(get_plash_data(), 'index', '0')
    if not os.path.exists(last_inited):
        die('first run `plash init`')


def run_write_read(cmd, input):
    import subprocess
    p = subprocess.Popen(cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE)
    p.stdin.write(input)
    p.stdin.close()
    exit = p.wait()
    if exit:
        raise subprocess.CalledProcessError(exit, cmd)
    return p.stdout.read()


def mkdtemp():
    import tempfile
    return tempfile.mkdtemp(
        dir=os.path.join(get_plash_data(), 'tmp'),
        prefix='plashtmp_{}_{}_'.format(os.getsid(0), os.getpid()))
