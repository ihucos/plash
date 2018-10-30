from setuptools import setup, Extension
import os

unshare_module = Extension('unshare',
                    sources=['plash/C/pyunshare.c'],
                    py_limited_api=True,
                    )

VERSION = '0.1dev'

setup(
    name='plash',
    version=VERSION,
    description='Container build and run tool',
    url='https://github.com/ihucos/plash',
    packages=['plash', 'plash.macros'],
    scripts=['bin/plash', 'bin/plash-exec'],
    zip_safe=False,
    include_package_data=True,
    ext_modules=[unshare_module],

    # extra stuff
    python_requires='>=3',
)
