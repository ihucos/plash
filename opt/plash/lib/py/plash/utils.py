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


def nodepath_or_die(container, allow_root_container=False):
    extra = [] if not allow_root_container else ['--allow-root-container']
    return plash_call('nodepath', str(container), *extra)


def get_default_shell(passwd_file):
    with open(passwd_file) as f:
        #  the first entry is the root entry
        #  https://security.stackexchange.com/questions/96241/why-require-root-to-be-the-first-entry-in-etc-passwd
        root_entry = f.readline().rstrip('\n')
        default_root_shell = root_entry.split(":")[6]
        return default_root_shell


def get_default_user_shell():
    import pwd
    return pwd.getpwuid(os.getuid()).pw_shell


def plash_map(*args):
    from subprocess import check_output
    'thin wrapper around plash map'
    out = plash_call('map', *args)
    if out == '':
        return None
    return out


def assert_initialized():
    last_inited = join(os.environ["PLASH_DATA"], 'index', '0')
    if not os.path.exists(last_inited):
        die('first run `plash init`')


def mkdtemp():
    import tempfile
    return tempfile.mkdtemp(
        dir=os.path.join(os.environ["PLASH_DATA"], 'tmp'),
        prefix='plashtmp_{}_{}_'.format(os.getsid(0), os.getpid()))


def py_exec(file, *args):
        import runpy

        sys.argv = [sys.argv[0]] + list(args)

        #import time
        #t = time.time()
        runpy.run_path(file)
        #print(time.time() - t, file, *args, file=sys.stderr)

        sys.exit(0)


def plash_exec(plash_cmd, *args):

        thisdir = os.path.dirname(os.path.abspath(__file__))
        execdir = os.path.abspath(os.path.join(thisdir, '..', '..', 'exec'))
        runfile = os.path.join(execdir, plash_cmd)
        py_exec(runfile, *args)


def plash_call(plash_cmd, *args,
        strip=True, return_exit_code=False, stdout_to_stderr=False, input=None, cwd=None):
    r, w = os.pipe()

    if input:
        r2, w2 = os.pipe()

    child = os.fork()
    if not child:
        if cwd:
            os.chdir(cwd)
        if stdout_to_stderr:
            os.dup2(2, 1)
        else:
            os.dup2(w, 1)
        os.close(r)
        os.close(w)

        if input:
            os.dup2(r2, 0)
            os.close(r2)
            os.close(w2)

        plash_exec(plash_cmd, *args)

    os.close(w)

    if input:
        f = os.fdopen(w2, 'w')
        f.write(input)
        f.close()

    _, status = os.wait()
    exit = (status >> 8)
    # XXX check for abnormal exit

    if return_exit_code:
        return exit

    if exit:
        sys.exit(1)
    out = os.fdopen(r).read()
    if strip:
        out = out.strip('\n\r ')
    return out


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
        container_id = plash_call('build', *args)
        py_exec(sys.argv[0], container_id, *cmd)

