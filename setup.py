from setuptools import setup

setup(name='plash',
      version='0.2.0',
      description='Try new Linux distros',
      url='http://github.com/ihucos/plash',
      author='Irae Hueck Costa',
      author_email='irae.hueck.costa@gmail.com',
      license='MIT',
      packages=['plash'],
      scripts=['bin/plash', 'bin/plashexec'],
      package_dir={'plash': '.'},
      package_data={'plash': ['data/bin/*']},
      install_requires=[
      ],
      include_package_data=True,
      zip_safe=False)
