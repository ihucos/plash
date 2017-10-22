import os
from codecs import open
from os import path

from setuptools import find_packages, setup

setup(
      name='plash',
      version='0.6',
      license='MIT',
      # package_data={
      #     'sample': ['package_data.dat'],
      # },

      # Although 'package_data' is the preferred approach, in some case you may
      # need to place data files outside of your packages. See:
      # http://docs.python.org/3.4/distutils/setupscript.html#installing-additional-files # noqa
      # In this case, 'data_file' will be installed into '<sys.prefix>/my_data'
      # data_files=[('/opt/plash/bin', os.listdir('./bin'))],
      # data_files=[('/opt/plash/bin', os.listdir('./bin'))],
      data_files=[('/usr/local/bin', ['./plash', 'plash.rundata'])],
      zip_safe=False
)
