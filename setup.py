from os import listdir, path

from setuptools import setup

bins = listdir(path.join((path.dirname(__file__)), 'bin'))

setup(name='plash',
      version='0.2.0',
      description='Try new Linux distros',
      url='http://github.com/ihucos/plash',
      author='Irae Hueck Costa',
      author_email='irae.hueck.costa@gmail.com',
      license='MIT',
      packages=['plash'],
      scripts=[path.join('bin', b) for b in bins],
      install_requires=[
      ],
      include_package_data=True,
      zip_safe=False)
