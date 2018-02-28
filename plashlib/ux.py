from plashlib.utils import die, nodepath_or_die, get_plash_data
from os.path import join
import os


def assert_initialized():
    last_inited = join(get_plash_data(), 'index', '0')
    if not os.path.exists(last_inited):
        die('run plash-init first')


def assert_is_root():
    if not os.getuid() == 0:
        die('you are not root')


def assert_container_exists(cont):
    nodepath_or_die(cont)
