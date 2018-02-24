from setuptools import setup
from os import path
import os
import sys

VERSION = '0.1dev'

if os.environ.get('TRAVIS'):
    assert VERSION != '0.1dev', 'not building with version 0.1dev in travis'

# workaround, python packaging is terrible
if 'bdist_wheel' in sys.argv:
    raise RuntimeError("This setup.py does not support wheels")

here = path.abspath(path.dirname(__file__))

bin_files = set([
    path.join('bin', i) for i in os.listdir(path.join(here, 'bin'))
    if not '.' in i
])

# be sure about the access rights since this is security sensitive
for file in bin_files:
    os.chmod(file, 0o755)

# only automatically setuid when plash-pun is better reviewed
## This will implicitly change the access rights to setuid in dev
#os.chmod('bin/plash-pun', 0o4755)

setup(
    name='plash',
    version=VERSION,
    description='Container build and run tool',
    url='https://github.com/ihucos/plash',
    packages=['plashlib'],
    data_files=[("/usr/local/bin", bin_files)],

    # extra stuff
    python_requires='>=3',
)
