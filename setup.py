from setuptools import setup, find_packages
from setuptools.command.install import install
from subprocess import check_call
from codecs import open
from os import path
import os

here = path.abspath(path.dirname(__file__))

bin_files= set([
        path.join('bin', i)
        for i in os.listdir(path.join(here, 'bin'))
        if not '.' in i])

# be sure about the access rights since this is security sensitive
for file in bin_files:
    os.chmod(file, 0o755)

# This will implicitly change the access rights to setuid in dev
os.chmod('bin/plash-pun', 0o4755)

setup(
    name='plash',
    version='0.8',
    description='Container build tool',
    url='https://github.com/ihucos/plash',
    packages=['plashlib'],
    data_files=[("/usr/local/bin", bin_files)],
)
