# import where needed to make startup faster
import os
import sys
from contextlib import contextmanager
from os.path import join
import ctypes

ERROR_COLOR = 1


def hashstr(stri):
    import hashlib

    return hashlib.sha1(stri).hexdigest()


@contextmanager
def catch_and_die(exceptions, debug=None, debug_class=False, ignore=None, silent=False):
    try:
        yield
    except tuple(exceptions) as exc:
        if ignore and isinstance(exc, ignore):
            raise
        if silent:
            sys.exit(exit)
        msg = str(exc)
        if msg.startswith("<") and msg.endswith(">"):
            msg = msg[1:-1]
        if debug_class:
            debug = exc.__class__.__name__
        if debug:
            msg = "{debug}: {message}".format(debug=debug, message=msg)
        die(msg)


def info(msg):
    print(msg, file=sys.stderr)


def die_with_usage():
    prog = os.path.basename(sys.argv[0])
    printed_usage = False
    with open(sys.argv[0]) as f:
        for line in f.readlines():
            if line.startswith("# usage:"):
                usage_line = line[2:]
                print(usage_line, file=sys.stderr, end="")
                printed_usage = True
    assert printed_usage, "could not find usage comment"
    sys.exit(1)


def nodepath_or_die(container, allow_root_container=False):
    extra = [] if not allow_root_container else ["--allow-root-container"]
    return plash_call("nodepath", str(container), *extra)


def get_default_shell(passwd_file):
    with open(passwd_file) as f:
        #  the first entry is the root entry
        #  https://security.stackexchange.com/questions/96241/why-require-root-to-be-the-first-entry-in-etc-passwd
        root_entry = f.readline().rstrip("\n")
        default_root_shell = root_entry.split(":")[6]
        return default_root_shell


def get_default_user_shell():
    import pwd

    return pwd.getpwuid(os.getuid()).pw_shell


def plash_map(*args):
    "thin wrapper around plash map"
    out = plash_call("map", *args)
    if out == "":
        return None
    return out


def assert_initialized():
    plash_data = plash_call("data")
    last_inited = join(plash_data, "index", "0")
    if not os.path.exists(last_inited):
        die("first run `plash init`")


def py_exec(file, *args):
    import runpy

    sys.argv = [file] + list(args)
    runpy.run_path(file)
    sys.exit(0)


def plash_exec(plash_cmd, *args):

    # security validation!
    assert plash_cmd.replace("-", "").isalpha()

    thisdir = os.path.dirname(os.path.abspath(__file__))
    execdir = os.path.abspath(os.path.join(thisdir, "..", "..", "exec"))
    runfile = os.path.join(execdir, plash_cmd)
    with open(runfile, "rb") as f:
        is_python = f.read(23) == b"#!/usr/bin/env python3\n"
    if is_python:
        py_exec(runfile, *args)
    else:
        os.execlp(runfile, runfile, *args)


def plash_call(
    plash_cmd,
    *args,
    strip=True,
    return_exit_code=False,
    stdout_to_stderr=False,
    input=None,
    cwd=None
):
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
        f = os.fdopen(w2, "w")
        f.write(input)
        f.close()

    _, status = os.wait()
    exit = status >> 8
    # XXX check for abnormal exit

    if return_exit_code:
        return exit

    if exit:
        sys.exit(1)
    out = os.fdopen(r).read()
    if strip:
        out = out.strip("\n\r ")
    return out


dir_path = os.path.dirname(os.path.realpath(__file__))
clib = os.path.realpath(os.path.join(dir_path, "../../c/plash.o"))

lib = ctypes.CDLL(clib)


def unshare_user():
    lib.pl_unshare_user()


def unshare_mount():
    lib.pl_unshare_mount()


def die(msg):
    lib.pl_fatal(ctypes.c_char_p(msg.encode()))
