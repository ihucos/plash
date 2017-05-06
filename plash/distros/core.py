from plash.distros.base import OS

class Debian(OS):
    base_image= 'debian'
    packages = Apt.call('python-pip', 'software-properties-common')

class Ubuntu(OS):
    base_image = 'ubuntu:rolling'
    packages = 'rm /etc/apt/apt.conf.d/docker-clean && '+ Apt.call('python-pip', 'npm', 'software-properties-common')

class Centos(OS):
    base_image = 'centos'
    packages = Yum.call('epel-release', 'python-pip')

class Alpine(OS):
    name = 'alpine'
    base_image = 'alpine'

class Gentoo(OS):
    base_image = 'thedcg/gentoo'
    packages = Emerge.call('dev-python/pip')

