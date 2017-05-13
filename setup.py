from setuptools import setup

setup(name='plash',
      version='0.1.0',
      description='Try new Linux distros',
      url='http://github.com/ihucos/plash',
      author='Irae Hueck Costa',
      author_email='irae.hueck.costa@gmail.com',
      license='MIT',
      packages=['plash'],
      scripts=['bin/plash', 'bin/plashrun'],
      install_requires=[
            # 'PyYAML==3.12'
      ],
      include_package_data=True,
      zip_safe=True)
