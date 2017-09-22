## This README is old, documentation will be writte for the first stable release

# plash

Plash is a flexible build tool for docker images. Current version is 0.2 alpha.


## Install

*Api and configuration format unstable*

`pip3 install git+https://github.com/ihucos/plash.git`

## Crash Course

With plash you can run `nvim` without installing it to the operating system.
```
$ plash ubuntu :add-apt-repository ppa:neovim-ppa/stable :apt neovim -- nvim myfile
```
Your home directory is mounted as home into the container.
Building is cached, if you run this command twice, building will not happen again.

You can import command line arguments from files
```
$ plash @nvim
$ cat nvim
#!/usr/bin/env plashexec
ubuntu
nvim
:add-apt-repository ppa:neovim-ppa/stable
:apt neovim
```
Note the shebang, after marking this file named `nvim` executable it can be directly run and you could also put it into your `PATH`. The main idea of plash is to have programms that run what you need inside a container. Plashs vision of containers is that similiar to a chroot, only the file system is virtualized and other resources accesible. A plash script should behave like a "native" programm. (Currently plash is on top of docker, I want to change it to libcontainer/runc with an overlay file system)

Here is another simple example of a plash file:
```
:apt
	package1
	package2

:layer # layering is explicit

:run touch myfile
```

Plash scripts can be seen as one dimensional lisp, 'layer' for instance is actually a macro.
```
:layer apt
	package1
	package2
	package3
```
Results to the same as
```
:layer
:apt package1
:layer
:apt package2
:layer
:apt package3
:layer
```
Another macro is the action 'warp':
```
:warp run cp -r {./data_dir_at_host} /app/data
```

Buildtime mounts are supported
```
:mount .
:pwd .
:rebuild-when-changed
	myfile
	mydir
```

You can have buildtime arguments, like with all other actions rebuilding happens if necessary.
```
:import-env
  MYDIR
  PATH:HOST_PATH

:all run  # all applies each argument to its first arguments
	mkdir $MYDIR
	cd $MYDIR
```

Altough it can be done, plash does not try to be a general purpose language. You can define new actions that are external scripts.

```
:define touch
	touch $1	

:touch myfile

```
Or if you don't like bash:
```
:apt python
:define mkdir
	#!/usr/bin/env python
	import sys, os
	os.mkdir(sys.argv[1])
:mkdir mydir
```

But this is actually just for quick one-shot functions. You can implement new actions by importing python modules that have callables registered with the `plash.eval.register` decorator. See `stdlib.py` for examples.
```
plash ubuntu --no-stdlib :import myplashlib :funcyfunc
```

Only two actions are build in, `import` and `layer`. The rest comes from the stdlib which can be easily extended or replaced.


## Using plash to create docker images

Plash scripts can be saved to a docker image
```
$ plash @myplashscript --save-image myimage --build-only
$ docker push dockeruser485/myimage
$ plash myimage bash
```

## Use case example: virtualenv replacement
One of Python's virtualenvs shortcoming is that packages often can not be compiled at another computer. With plash we can isolate all dependencies inside a container and still be very leightweight on the development side.


```
#!/usr/bin/env plashexec

debian
python

:apt
	python
	python-pip
	python-dev
	binutils

:layer

:warp run pip install -r {myapp/requirements.txt}

:layer

:import-env MYAPP_DEVREQUIREMENTS
:warp script
	[ "$MYAPP_DEVREQUIREMENTS" = "1" ] && pip install -r \
	{myapp/devrequirements.txt}
	true
```
This could be inside an executable file named `python` or inside a bin folder in your project root and checked into version control.
That way you will have a python that executes your app inside a container with the required libraries. Note with this example how you can `export MYAPP_DEVREQUIREMENTS=1` in order to also get comforts like e.g. ipdb inside your container.



## Roadmap
This is an alpha release. The next step is to get a stable minimalistic stdlib that is easy to understand and fits the needs of building images.
I'd actually like to see docker only as a backend that can be used and actually rely on runc/libcontainer. The configuration management part of plash should be very leightweight, transparent and decopuled. I do not want to create the next ansible or turing complete scripting language. The vision of containers in plash is that they actually only isolate the file system, like a chroot. Other resources should be accesible inside the container. It should play well with the UNIX world, if you need something like `docker-compose`, use `supervisord`. If you need more isolation existing tools can be used.
