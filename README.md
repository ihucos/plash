[![Build Status](https://travis-ci.org/ihucos/plash.svg?branch=master)](https://travis-ci.org/ihucos/plash) 

# Plash

Build and run layered root filesystems and fulfill your miscellaneous container needs.


## Install / Uninstall
```
sudo sh -c "curl -Lf https://raw.githubusercontent.com/ihucos/plash/master/setup.sh | sh -s"
sudo rm -rf /usr/local/bin/plash /usr/local/bin/plash-exec /opt/plash/ #  uninstall
```

## Requirements
  - `python3`
  - Linux Kernel >= 4.18
  - `unionfs-fuse`, `fuse-overlayfs` or access to the kernel's builtin overlay filesystem
  - Optional `newuidmap` and `newgidmap` for setuid/setgid support with non-root users

## Documentation
```plash --from alpine --apk xeyes -- xeyes```

[See existing documentation here](http://plash.io/)


## Caveats

- Plash processes have the same operating system access rights than the process
  that started it. There is no security relevant isolation feature. Exactly as
  with running programs "normally", don't run programs you do not trust with
  plash and try to avoid using plash with the root user.

- Plash only runs inside Docker containers started with the `--privileged`
  flag, see GitHub issue #51 for details. 

- Nested plash instances are not possible with `unionfs-fuse` (#69).  But
  `fuse-overlayfs` and `overlay` work fine.


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

### How does the code look?
Some python3, some C. Very little code, very maintainable.

### How does plash compare to Docker?
Docker is a bloated SUV you have to bring to the car workshop every week, for
random alterations, features and new advertising stickers. Plash is a nice
fixed gear bike, but the welds are still hot and nobody checked the bolts yet.

### Can I run this in production?
You can. It probably still has some warts, what I can guarantee is to
enthusiastically support this software and all issues that may come with it and
focus on backward compatibility.

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
semi-isolated containers and behave oddly. Plash uses unmodified lxc images and
checks their signatures with gpgv (if in PATH). Using plash as root should be
avoided and should not be necessary for most use cases.  Until now plash was
written by one person and of course I could be wrong about something. But
generally speaking it really should be good enough.

### Why the unusual project structure?
Source code and packaged directory structure is the same to reduce complexity.
