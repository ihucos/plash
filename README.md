# plash

Plash is a swiss army knife for containers that easily turns into a Machete. Current version is 0.1 alpha.


## Install
`pip3 install git+https://github.com/ihucos/plash.git`



## Use Cases


### Get newer version of a package
Use a newer package than what your package manager provides.
```
$ plash --gentoo --emerge nmap -- nmap --version

Nmap version 7.40 ( https://nmap.org )
Platform: x86_64-pc-linux-gnu
[...]
```
To install gentoo's nmap into your system
```
$ plash --install /usr/local/bin/nmap --gentoo --emerge nmap -- nmap
Installed to /usr/local/bin/nmap
```


### Quickly have a throwaway linux
Your files including the `.bashrc` will be available.
You can play around with unknown linux distributions.
```
joe@macbook ~/mypwd $ uname
Darwin
joe@macbook ~/mypwd $ plash --centos
root@moby ~/mypwd $ uname
Linux
```

### Virtualenv replacement
One of Python's virtualenvs shortcoming is that packages often can not be compiled at another computer. With plash we can isolate all dependencies inside a container and still be very leightweight on the development side.

```
 $ plash --debian --apt binutils python-dev --pip-requirements ./requirements.txt --install ./python -- python
 Installed to ./python
```

This will give you an executable file named `./python` that can be commited to version control and should behave just like the python installed in your operating system. Behind the scenes the requierements are installed in an fresh debian system. Rebuilding happens automatically.




In case you wondered, the executable `./python` file is a wrapped plash call.
```
$ cat ./python
#!/bin/sh
plash --debian --apt binutils python-dev --pip-requirements ./requirements.txt -- python "$@"
```


### Dockerfile alternative

Plash can include command line arguments labeled as "actions" from YAML files.
```
 $ plash --ubuntu --include plash.yaml
```

#### Explicit layering
```
# a plash.yaml

- apt:
 - package1
 - package2

- layer

- eval: touch myfile
```
Again, that is essential the same as writing:
plash --ubuntu --apt package1 package2 --layer --eval touch myfile

#### Build time arguments
```
- import-envs: MYDIR
- eval:
  - mkdir $MYDIR
  - cd $MYDIR
```


#### Build time mounts
```
- mount: .
- pwd: .
- rebuild-when-changed:
  - myfile
  - mydir
```

#### includes
Includes can includes includes
```
- include: myfile
```


#### Executable configuratio files
If you like the idea, you can have configuration files that when executed run the machine they configure.
```
#!/bin/bash
# vim: set filetype=yaml:
./plash --ubuntu --include <(cat <<'YamlConfigDelimiter'
# ========== [YOUR CONFIG] ==============

- rebuild-when-changed: somethingconfig
- mount: .
- pwd: .
- eval: make install

# ======================================
YamlConfigDelimiter
) -- ./myprogramm # <-- command to execute here
```
