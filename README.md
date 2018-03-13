[![Travis branch](https://img.shields.io/travis/ihucos/plash/master.svg?style=flat-square)](https://travis-ci.org/ihucos/plash#)
[![PyPI](https://img.shields.io/pypi/v/plash.svg?style=flat-square)](https://pypi.org/project/plash/)
# plash
is a container build and runtime system.

## Install
```
python3 -m pip install plash
```

## Why Plash?

#### Runs anywhere
Plash's only requirements are python3, a linux kernel (>= 3.18) and a rudimentary mount binary in `$PATH`. It does not need an extra daemon and can be easily run in infrastructure not meant to support containers like virtually any ci environment, embedded systems or even docker containers.

#### Security
Plash can be used completely unprivileged (with `unionfs-fuse` and `newuidmap` as dependencies)

#### Its just processes
Plash containers are processes exactly like you know them. They can be listed with ps, `kill`ed, you can filter for stderr or pipe to stdin, manage them in groups with `supervisord` and `runit` or simply access files in your home directory. Only parts of the filesystem are isolated. More isoalition could be provided by seperate tools.

#### Plashfiles
Plashfiles are executable build files featuring optional lightweight configuration management capabilities.

## Documentation
* Reference: https://ihucos.github.io/plash

## Example session

```
# don't forget to run init first.
$ plash init

# build a simple image
$ plash build --image alpine --run 'touch /file'
[0%|10%|20%|30%|40%|50%|60%|70%|80%|90%|100%]
extracting...
--> touch /file
--:
2

# second build is cached
$ plash build --image alpine --run 'touch /file'
2

# layering is explicit
$ plash build --image alpine --run 'touch /file' --layer --run 'touch /file2'
--> touch /file2
--:
3

# delete a container
$ plash rm 3

# build and run in one command
$ plash run --image alpine --run 'touch /file' -- ls /file
/file

# plash actually includes some configuration management
$ plash run --image alpine --apk git -- git --version
--> apk update
fetch http://dl-cdn.alpinelinux.org/alpine/v3.7/main/x86_64/APKINDEX.tar.gz
<snip>
(6/6) Installing git (2.15.0-r1)
Executing busybox-1.27.2-r8.trigger
Executing ca-certificates-20171114-r0.trigger
OK: 21 MiB in 22 packages
--:
git version 2.15.0
```

## Other topics

### Plashfiles
TODO

### Philosophy
Plash thrives to not be more than a useful tool. There is no monetization strategy, bundling or closed source. It plays well with other software in the ecosystem. Plash and its internal architecture tries to honor the "Do One Thing and Do It Well" UNIX philosophy. This software tries to not "oversell" its abstraction layer, which is containerization of processes. Containerization and isolation are seen as two different tasks, but as a flexible tool this software does not enforce any specific why of using it. Sloppily speaking: after we had a revolutionary containerization of processes with docker, this is also some needed fine adjusting: a *processisation of containers*.
Plash tries to be very maintainable and is designed to still securely run without any changes in 5 or 10 years. This software tries to be in every aspect of the word lightweight. For example by covering less complicated use cases for
containers, like letting a user quickly use a newer software package from another distribution in a sane way. Or being easy to install. Last but not least in a new and fast-moving container world the focus is on boring long term stability and backwards compatibility, starting from version 1.0.

## FAQ

#### Can I contribute?
Please! Write me an mail plash@irae.me, open an issue, do a pull request or or ask me out for a friendly chat about plash in Berlin.

#### Who are you?
A Django/Python software-developer. Since this is an open source project I hope this software grows organically and collaboratively.

#### Why did you write it?
In my personal opinion Docker is revolutionary but has some shortcomings: awkward interface, reinvention of established software or interfaces, bundling, vendor lock in and overengineering. In a way it kills it's idea by trying too hard to build a huge company on top of it. Plash thrives to not to be more than a useful tool with one task: Building and running containerized processes. Ultimately I wanted something I can if necessary maintain by myself.

#### How does the code base look?
One python3 library without dependencies and one python3 script per command.

#### It's actually just a chrooted process?
Yes, also a container build engine, frameworkish features, some new ideas, some glue and technical subtleties.

#### Is this a replacement for docker?
No. There are no efforts to implement features like image distribution, process management, or orchestration in plash.  Instead it works nicely with docker or other software in the ecosystem. You can use plash to run docker in your system or run your plash containers inside docker. If you for exampling are using docker but want to have containers inside your docker container, you could use plash. You can also access docker images from plash or export plash images to docker. So another use case is to build docker images inside docker containers. They are two software with completely different philosophies. In my opinion docker is kind of a big framework that tries to solve all your container related problems. Plash on the other hand tries to be a flexible minimalistic command line tool that is only concerned with the essential needs of using containers. Both approaches are totally valid and very different. Comparing the code base between the two projects is also difficult, plash is currently only developed by one person.

#### Can I run this in production?
No. Not yet.

#### When will be the >= 1.0 release?
Will still take some time, after this:
- [ ]  Better documentation
~~- [ ] Security audit - no suid binaries anymore ~~
- [ ] API worth staying
- [ ] Some little real-world usage
- [ ] More unit tests
