from setuptools import setup
import os

VERSION = '0.1dev'

if os.environ.get('TRAVIS'):
    assert VERSION != '0.1dev', 'not building with version 0.1dev in travis'

setup(
    name='plash',
    version=VERSION,
    description='Container build and run tool',
    url='https://github.com/ihucos/plash',
    packages=['plashlib'],
    scripts=['bin/plash', 'bin/plash-exec'],
    zip_safe=False,
    include_package_data=True,

    # extra stuff
    python_requires='>=3',
)
