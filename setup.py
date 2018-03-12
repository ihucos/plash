from setuptools import setup
from os import path
import os
import sys

VERSION = '0.1dev'

if os.environ.get('TRAVIS'):
    assert VERSION != '0.1dev', 'not building with version 0.1dev in travis'

setup(
    name='plash',
    version=VERSION,
    description='Container build and run tool',
    url='https://github.com/ihucos/plash',
    packages=['plashlib'],
    scripts=['bin/plash'],
    zip_safe=False,
    include_package_data = True,

    # extra stuff
    python_requires='>=3',
)
