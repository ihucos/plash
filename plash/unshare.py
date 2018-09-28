#!/usr/bin/env python3
#
# usage: plash noroot CMD

import ctypes
import errno
import os
import signal
import atexit
import sys
from getpass import getuser
from multiprocessing import Lock  # that takes way too long to load
from subprocess import CalledProcessError, check_call

from plash import utils
from plash.utils import catch_and_die, die, get_plash_data

# I do believe this libc constants are stable.
CLONE_NEWNS = 0x00020000
CLONE_NEWUSER = 0x10000000
MS_REC = 0x4000
MS_PRIVATE = 1 << 18


def die_with_errno(calling, extra=''):
    myerrno = ctypes.get_errno()
    errno_str = errno.errorcode.get(myerrno, myerrno)
    die('calling {} returned {} {}'.format(calling, errno_str, extra))


def get_subs(query_user, subfile):
    'get subuids or subgids for a user'
    try:
        with open(subfile) as f:
            read = f.readline()
            user, start, count = read.split(':')
            if user == query_user:
                return int(start), int(count)
    except FileNotFoundError:
        pass
    die('please configure a subgid/uid range for the user {} in /etc/subuid and /etc/subgid'.
        format(repr(query_user)))


def unshare_if_user():
    '''
    Exits the program on failure
    '''

    if not os.getuid():
        return
    os.environ['PLASH_DATA'] = get_plash_data()
    uid_start, uid_count = get_subs(getuser(), '/etc/subuid')
    gid_start, gid_count = get_subs(getuser(), '/etc/subgid')

    setup_user_map = [[
        'newuidmap',
        str(os.getpid()), '0',
        str(os.getuid()), '1', '1',
        str(uid_start),
        str(uid_count)
    ],
                  [
                      'newgidmap',
                      str(os.getpid()), '0',
                      str(os.getgid()), '1', '1',
                      str(gid_start),
                      str(gid_count)
                  ]]

    def prepare_unshared_proccess():
        for cmd in setup_user_map:
            try:
                with catch_and_die([CalledProcessError],
                                   debug='forked child'):
                    check_call(cmd)
            except FileNotFoundError as exc:
                utils.die('newuidmap/newgidmap not found in PATH, please install it (package typically called `uidmap` or `shadow-utils`)')

    # we need to call prepare_unshared_proccess
    # from outside of the unshared process
    lock = Lock()
    lock.acquire()
    child = os.fork()
    if not child:
        lock.acquire()
        prepare_unshared_proccess()
        sys.exit(0)

    @atexit.register
    def kill_child():
        os.kill(child, signal.SIGKILL)

    # what the unshare binary does do
    libc = ctypes.CDLL('libc.so.6', use_errno=True)
    libc.unshare(CLONE_NEWUSER) != -1 or die_with_errno(
        '`unshare(CLONE_NEWUSER)`',
        '(maybe try `sysctl -w kernel.unprivileged_userns_clone=1`)')
    libc.unshare(CLONE_NEWNS) != -1 or die_with_errno('`unshare(CLONE_NEWNS)`')
    libc.mount("none", "/", None, MS_REC | MS_PRIVATE,
               None) != -1 or die_with_errno('mount')

    atexit.unregister(kill_child)

    lock.release()
    pid, raw_exit_status = os.wait()
    exit_status = raw_exit_status // 255
    if exit_status:
        sys.exit(1)


def unshare_if_root():
    if os.getuid() or os.environ.get('PLASH_NO_UNSHARE'):
        return
    libc = ctypes.CDLL('libc.so.6', use_errno=True)

    libc.unshare(CLONE_NEWNS) != -1 or die_with_errno('`unshare(CLONE_NEWNS)`')
    libc.mount("none", "/", None, MS_REC | MS_PRIVATE,
               None) != -1 or die_with_errno('mounting')
