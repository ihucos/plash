from setuptools import setup, find_packages
from setuptools.command.install import install
from subprocess import check_call
from codecs import open
from os import path
import os

here = path.abspath(path.dirname(__file__))

class PostInstallCommand(install):     
    def run(self):   
        install.run(self)
        print()
        print('##  calling plash-run-suid-install ##')
        #check_call(['plash-run-suid-install'])
        os.system('ash')

bin_files=[path.join('bin', i) for i in os.listdir(path.join(here, 'bin'))]
for file in bin_files:
    print(file)

setup(
    name='plash',
    version='0.8',
    description='Container build tool',
    url='https://github.com/ihucos/plash',
    packages=['plashlib'],
    data_files=[("/usr/local/bin", bin_files)],
    cmdclass={'install': PostInstallCommand},
)
