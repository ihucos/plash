[![Build Status](https://travis-ci.org/ihucos/plash.svg?branch=master)](https://travis-ci.org/ihucos/plash) 

# Plash

Build and run layered root filesystems.


## Install / Uninstall
```
sudo sh -c "curl -Lf https://raw.githubusercontent.com/ihucos/plash/master/setup.sh | sh"

# uninstall
sudo rm -rf /usr/local/bin/plash /usr/local/bin/plash-exec /opt/plash/
```

## Requirements
  - `python3`, `bash`, `make` and `cc`
  - Linux Kernel >= 4.18
  - `unionfs-fuse`, `fuse-overlayfs` or access to the kernel's builtin overlay filesystem
  - Optional `newuidmap` and `newgidmap` for setuid/setgid support with non-root users

## Documentation
```plash --from alpine --apk xeyes -- xeyes```

https://ihucos.github.io/plash-docs-deploy/


## Caveats

- Plash processes have the same operating system access rights than the process
  that started it. There is no security relevant isolation feature. Exactly as
  with running programs "normally", don't run programs you do not trust with
  plash and try to avoid using plash with the root user.

- Plash only runs inside Docker containers started with the `--privileged`
  flag, see GitHub issue #51 for details. 

- Nested plash instances are not possible with `unionfs-fuse` (#69).  But
  `fuse-overlayfs` and `overlay` work fine.
  
## Plash vs. Other Container Engines

Plash containers are not necessarily true containers because they do not fully isolate themselves from the host system and do not have additional security measures set in place. Instead, they are more like a combination of processes and containers, and can be treated like normal processes (e.g., they can be killed). Plash containers also have access to the home directory of the user who started them. To better understand this concept, refer to the provided diagram. 

```
Threads < Processes < Plash < Containers < Virtualisation < Physical computer
```

In general, the more to the left something is on the spectrum, the less flexible it is, but the more integrated it is with the system, allowing it to share existing resources. Plash containers are more constrained than traditional containers, but in exchange, they have access to resources that would typically only be available to processes.

### Resources plash containers share with it’s caller:
- Network access, including access to the hosts localhost
- The user's home directory
- The tmp directory (during runtime, not during building)
- The Linux kernel (as with traditional containers)

### Resources unique to a plash containers
- The mount namespace
- The root folder, allowing running a different linux distribution

### Let’s look at an example.

I want to edit an image at my Desktop with the gimp image editor.

$ plash build --from alpine:edge --apk gimp
112
$ plash run 112 gimp

Gimp automatically has access to my X-Server and pop ups. It also has access to my home folder and all my files. But it does run on an alpine distribution and pretty much only in that regard is independent from my host operating system.



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
- The right guidelines for the right situation.


## User Interface Guidelines
- Interface follows code
- Code supplements documentation
- Documentation compensates a raw user interface
- Put effort into documentation
- If you want fun, go play outside
- Focus on expert users and automated systems as CLI consumers
- Don't make difficult things seem easy
- Don't be too verbose, usually only information about success or failure matter
- plash will be learned once but used multiple times
- Avoid too many features slowly getting in
- The UI is not a marketing instrument
- Ugly wards testify emphasis on backward compatibility
- Learning plash should be a valuable skill that lasts
- Users don't know what they want
- user errors are the user's fault
- Rude is better than sorry
- Technical descriptions do not get outdated, reasoning and interpretations do.

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
semi-isolated containers and behave oddly. Plash uses unmodified lxc images.
Using plash as root should be avoided and should not be necessary for most use
cases.  Until now plash was written by one person and of course I could be
wrong about something. But generally speaking it really should be good enough.
