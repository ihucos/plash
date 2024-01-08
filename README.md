[![Build Status](https://travis-ci.org/ihucos/plash.svg?branch=master)](https://travis-ci.org/ihucos/plash) 

# Plash

Build and run layered root filesystems.

```
USAGE: plash ...

  Import Image:
    [cached] pull:docker IMAGE[:TAG]  -  Pull image from docker cli
    [cached] pull:lxc DISTRO:VERSION  -  Download image from images.linuxcontainers.org
    [cached] pull:tarfile ARG         -  Import the image from an file
    [cached] pull:url ARG             -  Download image from an url

  Export Image:
    [noid] push:dir [ID] ARG          -  Export image to a directory
    [noid] push:tarfile [ID] ARG      -  Export image to a file

  Image Commands:
    [noid] [cached] create [ID] CODE  -  Create a new image
    [noid] mount [ID] MOUNTDIR        -  Mount image to the host filesystem
    [noid] mounted [ID] [CMD ...]     -  Run command on a mounted image
    [noid] nodepath [--allow-0] [ID]  -  Print filesystem path of an image
    [noid] parent [ID]                -  Print the parents image
    [noid] rm [ID]                    -  Remove image and its children
    [noid] run [ID] [CMD ...]         -  Run command in image
    [noid] stack [ID] DIR             -  Create a new image specyfing its layer

  Other Commands:
    clean         -  Remove internal unsused files
    mkdtemp       -  Create tempory data directory
    data          -  Print application data path
    purge         -  Remove all application data
    shrink        -  Remove half of all images
    help          -  print help message
    map KEY [ID]  -  map lorem ipsum
    sudo ...      -  run program as 'userspace root'
    version       -  print version
    init          -  initialize data dir
```


## Install
```
cd $(mktemp -d)
git clone git@github.com:ihucos/plash.git .
make
sudo cp dist/plash /usr/local/bin
```

## Uninstall
```
sudo rm /usr/local/bin/plash
```

## Requirements
  - `make` and `cc`
  - Linux Kernel >= 5.11
  - Optional `newuidmap` and `newgidmap` for setuid/setgid support with non-root users (needed by e.G. `apt`)

## Caveats

- Plash processes have the same operating system access rights than the process
  that started it. There is no security relevant isolation feature. Exactly as
  with running programs "normally", don't run programs you do not trust with
  plash and try to avoid using plash with the root user.

- Plash only runs inside Docker containers started with the `--privileged`
  flag, see GitHub issue #51 for details. 
  
## Plash vs Other Container Engines

Plash containers are not necessarily true containers because they do not fully isolate themselves from the host system and do not have additional security measures set in place. Instead, they are more like a combination of processes and containers, and can be treated like normal processes (e.g., they can be killed). Plash containers also have access to the home directory of the user who started them. To better understand this concept, refer to the provided diagram. 

```
Threads < Processes < Plash < Containers < Virtualisation < Physical computer
```

In general, the more to the left something is on the spectrum, the less flexible it is, but the more integrated it is with the system, allowing it to share existing resources. Plash containers are more constrained than traditional containers, but in exchange, they have access to resources that would typically only be available to processes.

### Resources plash containers share with it's caller
- Network access, including access to the hosts localhost
- The user's home directory (/home is mapped)
- The /tmp directory (during runtime, not during building)
- The Linux kernel (as with traditional containers)

### Resources unique to a plash containers
- The mount namespace
- The root folder, allowing running a different linux distribution


Plash containers are just a normal Linux process that happen to run on a different root filesystem. This means that they have their own set of benefits and drawbacks and may be more or less suitable for a particular use case.

## User Interface Guidelines
- Elegance in minimalism
- User interface needs break Development Guidelines

## Development Guidelines

- Keep the script character.
- Don't fall in love with the code, embrace its absence.
- All dependencies will get unmaintained at some point.
- Use honest thin wrappers, documented leaky abstractions are better then difficult promises.
- Don't be a monolith but don't try too hard not to be one.
- Don't complain or warn via stderr, do it or don't do it.
- Only be as smart as necessary and keep it simple and stupid (KISS).
- Still be able to run this in five years without any maintenance work.
- No baggage, no worries.
- Define well what this project is and especially what it is not.
- Say no to features, say yes to solved use cases.
- Postpone compromises.
- Ditch everything that turns out too fiddly.
- Be as vanilla as you can be
- Be humble, don't oversell your abstraction layer.
- Sometimes the dirty solution is cleaner than the proper one.
- Don't differentiate root from non-root users (this is a TODO)
- Crude is better than complex.
- Only eat your own dog food if you are hungry.
- Work towards a timeless, finished product that will require no maintenance.
- Don't write C just because it looks cool, use the right tool for the right job.
- Cognitive load for endusers does matter after all
- The right guidelines for the right situation.


## FAQ

### Can I contribute?
Please! Write me an mail mail@irae.me, open an issue, do a pull request or ask
me out for a friendly chat about plash in Berlin.

### Who are you?
A Django/Python software-developer. Since this is an open source project I hope
this software grows organically and collaboratively.

### Why write a containerization software?
Technical idealism. I wanted a better technical solution for a problem. In my
personal opinion Docker is revolutionary but has some shortcomings: awkward
interface, reinvention of established software or interfaces, bundling, vendor
lock in and overengineering. In a way it kills it's idea by trying too hard to
build a huge company on top of it. Plash thrives not to be more than a useful
tool with one task: Building and running containerized processes. Ultimately I
wanted something I can if necessary maintain by myself.

### Are there plans to commercialise this?
No, there isn't. At the same time I don't want to risk disappointing anyone and
am not making any absolute guarantees.

### What is the Licence?
plash is licensed under the MIT Licence.

### How does plash compare to Docker?
Docker is a bloated SUV you have to bring to the car workshop every week, for
random alterations, features and new advertising stickers. Plash is a nice
fixed gear bike, but the welds are still hot and nobody checked the bolts yet.

### Can I run this in production?
No guarantees.

### Is plash secure?
Plash does not use any daemons or have its own setuid helper binaries. Note
that plash does not try to isolate containers (which are just normal
processes). That means that running a program inside plash is not a security
feature. Running any container software introduces more entities to trust, that
is the root file system image with its additional linux distribution and its
own package manager. Using a program from alpine edge could be considered less
secure than a package from debian stable or vice versa. Also note that keeping
containers updated is more difficult than keeping "normal" system software
updated. Furthermore note that programs could be not used to run inside
semi-isolated containers and behave oddly. Plash uses unmodified lxc images.
Using plash as root should be avoided and should not be necessary for most use
cases.  Until now plash was written by one person and of course I could be
wrong about something. But generally speaking it really should be good enough.
