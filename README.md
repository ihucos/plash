[![Build Status](https://travis-ci.org/ihucos/plash.svg?branch=stable1)](https://travis-ci.org/ihucos/plash)

Install

```
# TODO: check that
$ git clone https://github.com/ihucos/plash.git /opt/plash
$ echo 'PYTHONPATH=/opt/plash:$PYTHONPATH PATH=/opt/plash/bin:$PATH plash "$@"' > /usr/local/bin/plash
$ chmod +x /usr/local/bin/plash
```



Table of contents
=================

## Exit status codes

| Exit code | Description |
| --- | --- |
| 1 | Generic error code |
| 2 | Unrecognized arguments |
| 3 | Container does not exist |

## Subcommands

* [add-layer](#add-layer)
* [build](#build)
* [getscript](#getscript)
* [do](#do)
* [export-tar](#export-tar)
* [import-linuxcontainers](#import-linuxcontainers)
* [import-tar](#import-tar)
* [import-url](#import-url)
* [init](#init)
* [mount](#mount)
* [nodepath](#nodepath)
* [purge](#purge)
* [rm](#rm)
* [run](#run)
* [runp](#runp)


### add-layer
```
plash add-layer CONTAINER
```
Reads a shell script from stdin and returns a builded or cached container on top of the supplied container.

The new container is the only thing printed to stdout, building information and status messages are printed to stderr.

For must cases you would, use `plash build` for a higher level interface.

exit status:
same as `plash build`


### build
```
plash build <ACTIONS>
```
Builds a container. All command line options are interpreted as action calls from the `stdlib`. See insertdoclinkhere for available actions

Some short examples:
```
plash build -o zesty --run 'touch a'
plash build -o zesty --run 'touch a' --layer --run 'touch b'
plash build -o zesty --apt nmap
```

exit status:
1: Generic error code
2: Unrecognized arguments
3: Returning container from cache, not building
4: Build error - building returned non-zero exit status

##### Additional exit codes
| Exit code | Description |
| --- | --- |
| 4 | Returning container from cache, not building |
| 5 | Build error - building returned non-zero exit statust |


### getscript
```
plash getscript <ACTIONS>
```
Prints the shell script generated from actions passed as command line parameters.
See insertdoclinkhere for available actions

#### do


### export-tar
```
plash export-tar CONTAINER [FILE]
```
Export the file system of a container to the given file as a compressed tar archive. If no file is supplied or the file is '-' the tar archive wil be printed to stdout instead.

### import-linuxcontainers
```
plash import-linuxcontainers IMAGE-NAME
```
Pull an image from linuxcontainers.org

### import-tar
```
plash import-tar TARFILE CONTAINER-ID
```
Create a container from a tar file named by the `container-id` parameter.

### import-url
```
plash import-url URL IMAGE-ID
```
Create a container from an url pointing to a tar file. The container id is specified by the `container-id` parameter.


#### init

### mount
```
plash mount [--upperdir UPPERDIR] [--workdir WORKDIR] CONTAINER MOUNTPOINT
```
Mounts the filsystem of a container. To cleanup, unmout it with `umount` later.
The optional arguments are options passed to the overlay filesystem program. `upperdir` will include any changes made on the mountpoint, `workdir` is used internally by the `overlay` programm and must be located in the same file system device than `upperdir`.


### nodepath
```
plash nodepath CONTAINER
```
Prints the location of the last layer of container. Useful to for example debug what changes some layer does to the filesystem:
plash.nodepath $container | xargs tree

### purge
```
plash purge
```
Prompts to delete all build data.

### rm
```
plash rm [CONTAINER1 [CONTAINER2 [CONTAINER3 ...]]]
```
Deletes the given containers.

### run
```
plash run container [--workdir VALUE] [--upperdir VALUE] [CMD] [ARG1 [ARG2 ...]]
```
Run a command inside the container. If no command is specified, the container id is printed. 
The workdir and upperdir parameters can be used to save file system changes made inside the container.

### runp
```
plash runp CONTAINER [ARG1 [ARG2 [ARG3...]]
```
Run a container with runp.
See the runp project page: https://github.com/ihucos/runp
