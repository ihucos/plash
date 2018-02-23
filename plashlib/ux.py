from plashlib.utils import die, nodepath_or_die
from os.path import join
import os

PLASH_DATA = os.environ.get('PLASH_DATA', '/var/lib/plash')


def assert_initialized():
    return #XXXXXXXX
    last_inited = join(PLASH_DATA, 'index', '0')
    if not os.path.exists(last_inited):
        die('run plash-init first')


def assert_is_root():
    return
    if not os.getuid() == 0:
        die('you are not root')


def assert_container_exists(cont):
    nodepath_or_die(cont)
