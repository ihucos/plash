from setuptools import setup, find_packages
from codecs import open
from os import path
import os

here = path.abspath(path.dirname(__file__))

setup(
    name='plash',
    version='0.8',
    description='Container build tool',
    url='https://github.com/ihucos/plash',
    packages=['plashlib'],
    scripts=[path.join('bin', i) for i in os.listdir(path.join(here, 'bin'))],
)
