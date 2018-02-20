from setuptools import setup, find_packages
from setuptools.command.install import install
from subprocess import check_output
from codecs import open
from os import path
import os

DEFAULT_MINOR_VERSION = "8"

here = path.abspath(path.dirname(__file__))

bin_files= set([
        path.join('bin', i)
        for i in os.listdir(path.join(here, 'bin'))
        if not '.' in i])

# be sure about the access rights since this is security sensitive
for file in bin_files:
    os.chmod(file, 0o755)

# only automatically setuid when plash-pun is better reviewed
## This will implicitly change the access rights to setuid in dev
#os.chmod('bin/plash-pun', 0o4755)

setup(
    name='plash',
    version='0.{}'.format(os.environ.get('TRAVIS_BUILD_NUMBER', DEFAULT_MINOR_VERSION)),
    description='Container build and run tool',
    url='https://github.com/ihucos/plash',
    packages=['plashlib'],
    data_files=[("/usr/local/bin", bin_files)],
     
    # extra stuff
    python_requires='>=3',
)
