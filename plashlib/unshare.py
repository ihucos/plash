#!/usr/bin/env python3
#
# usage: plash-noroot CMD

import os
import sys
import signal
from subprocess import check_call, CalledProcessError
from plashlib.utils import catch_and_die, get_plash_data, die
import tempfile
from getpass import getuser
from multiprocessing import Lock # that takes way too long to load
import signal
import ctypes

# I do believe this libc constants are stable and pray every day for that
CLONE_NEWNS = 0x00020000
CLONE_NEWUSER = 0x10000000
MS_REC = 0x4000
MS_PRIVATE = 1<<18


def get_subs(query_user, subfile):
    'get subuids or subgids for a user'
    with open(subfile) as f:
        read = f.readline()
        user, start, count = read.split(':')
        if user == query_user:
            return int(start), int(count)
    die('The user {} does not havy any subuids or subgids, please add some'.format(query_user))

def unshare_if_user(extra_setup_cmd=None):
    if not os.getuid():
        return
    os.environ['PLASH_DATA'] = get_plash_data()
    uid_start, uid_count = get_subs(getuser(), '/etc/subuid')
    gid_start, gid_count = get_subs(getuser(), '/etc/subgid')
    
    setup_cmds = [
         [
              'newuidmap',
              str(os.getpid()),
              '0', str(os.getuid()), '1',
              '1', str(uid_start), str(uid_count)
         ], [
              'newgidmap',
              str(os.getpid()),
              '0', str(os.getgid()), '1',
              '1', str(gid_start), str(gid_count)]]

    if extra_setup_cmd:
        setup_cmds.append(extra_setup_cmd)
    
    def prepare_unshared_proccess():
        for cmd in setup_cmds:
            with catch_and_die([CalledProcessError], debug='forked child'):
                check_call(cmd)

    # we need to call prepare_unshared_proccess
    # from outside of the unshared process
    lock = Lock()
    lock.acquire()
    child = os.fork()
    if not child:
        lock.acquire()
        prepare_unshared_proccess()
        sys.exit(0)
    # what the unshare binary does do
    libc = ctypes.CDLL('libc.so.6')
    assert libc.unshare(CLONE_NEWNS | CLONE_NEWUSER) != -1
    assert libc.mount("none", "/", None, MS_REC|MS_PRIVATE, None) != -1

    lock.release()
    os.wait()

def unshare_if_root():
    if os.getuid():
        return
    libc = ctypes.CDLL('libc.so.6')
    assert libc.unshare(CLONE_NEWNS) != -1
    assert libc.mount("none", "/", None, MS_REC|MS_PRIVATE, None) != -1
