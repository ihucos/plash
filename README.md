
# Plash
is a container build an runtime system.

## install
```
python3 -m pip install https://github.com/ihucos/plash/archive/master.zip
```

## Documentation
* This README
* Reference: *link coming soon*
* Wiki pages in progress.


## Why Plash?

- **Runs anywhere**
Plash's only requirements are python3, a linux kernel (>= 3.18) and a rudimentary mount binary in $PATH. It does not need an extra daemon and can be easily run in infrastructure not meant to support containers like virtually any ci environment, embedded systems or even docker containers.

- **Flexibility**
You can mount a container filesystem, export/import docker images, run containers as chrooted processes or runc, directly add a raw layer o top of a container, save containers by a cache key and much more.

- **Its just processes**
Plash containers are processes exactly like you know them. They can be listed with ps, `kill`ed, you can filter for stderr or pipe to stdin, manage them in groups with supervisord or runit or simply access files in your home directory. Only parts of the filesystem are isolated. If you need more isolation, use another tool just for that or run containers "traditionally" with plash-runc.

- **Plashfiles**
Plashfiles are executable build files featuring optional lightweight configuration management capabilities.


## Example session
{{ EXAMPLE_HERE }}

## Other topics

### Security
Plash's default container runtime does come with a security concept. It does not try to reinvent any security layer but to only rely on traditional UNIX security mechanisms. Figuratively speaking plash does not try to barricade processes with tape. Inside a container a user is what he was outside of it. As far as the kernel cares the only difference between a container and a "normal" process is that the container is chrooted. Access rights established outside of the container are still valid inside. So in fact to install software with a package manager inside a container you probably need root, but to later run it not. It can be seen as a one to one mapping between host and container.
There is some fine print: Root access is needed for container setup. In the future the plan is to use suid binaries to archive this. At the moment this is done with sudo. To allow a user to use plash put this is the suoders file `joe ALL=NOPASSWD: /usr/local/bin/plash-run`. This should be considered unsafe until version 1.0.

### Plashfiles
TODO

### Philosophy
Plash thrives to not be more than a useful tool. There is no monetization strategy, bundling or closed source. It plays well with other software in the ecosystem. Plash and its internal architecture tries to honor the "Do One Thing and Do It Well" UNIX philosophy. This software tries to not "oversell" its abstraction layer, which is containerization of processes. Containerization and isolation are seen as two different tasks, but as a flexible tool this software does not enforce any specific why of using it. Sloppily speaking: after we had a revolutionary containerization of processes with docker, this is also some needed fine adjusting: a <b>processisation of containers</b>.

Plash tries to be very maintainable and is designed to still securely run without any changes in 5 or 10 years. This software tries to be in every aspect of the word lightweight. For example covering less complicated use cases for

containers, like letting a user quickly use a newer software package from another distribution in a sane way. Or being easy to install. Last but not least in a new and fast-moving container world the focus is on boring long term stability and backwards compatibility, starting from version 1.0.

## FAQ
TODO
