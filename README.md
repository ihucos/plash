[![Build Status](https://travis-ci.org/ihucos/plash-svg?branch=stable1)](https://travis-ci.org/ihucos/plash)

```
#
# Install 
#
$ pip3 install git+https://github.com/ihucos/plash.git


#
# Examples 
#

# build a simple image
$ plash-build --os ubuntu --run 'touch /file'
Container not found, trying to pull it
[0%|10%|20%|30%|40%|50%|60%|70%|80%|90%|100%]
--> touch /file
--:
a64

# second build is cached
$ plash-build --os ubuntu --run 'touch /file'
a64

# run something inside a container
$ plash-run a64 file /file
/file: empty

# layering is explicit
$ plash-build --os ubuntu --run 'touch /file' --layer --run 'touch /file2'
--> touch /file2
--:
858

# delete a container
$ plash-rm 858

# build and run in one command
$ plash-run --os ubuntu -run 'touch /file' -- file /file
/file: empty

# you can easily mount, execute a command on the filesystem, then unmount.
$ plash-with-mount a64 ls
bin  boot  dev  etc  file  home  lib  lib64  media  mnt  opt  plashenv  proc  root  run  sbin  srv  sys  tmp  usr  var
$ plash-with-mount a64 chroot .
root@irae:/# exit

# plash actually includes some configuration management 
$ plash-run --os alpine --apk git -- git --version
--> apk update
fetch http://dl-cdn.alpinelinux.org/alpine/v3.7/main/x86_64/APKINDEX.tar.gz
fetch http://dl-cdn.alpinelinux.org/alpine/v3.7/community/x86_64/APKINDEX.tar.gz
v3.7.0-50-gc8da5122a4 [http://dl-cdn.alpinelinux.org/alpine/v3.7/main]
v3.7.0-49-g06d6ae04c3 [http://dl-cdn.alpinelinux.org/alpine/v3.7/community]
OK: 9044 distinct packages available
--> apk add git
(1/6) Installing ca-certificates (20171114-r0)
(2/6) Installing libssh2 (1.8.0-r2)
(3/6) Installing libcurl (7.57.0-r0)
(4/6) Installing expat (2.2.5-r0)
(5/6) Installing pcre2 (10.30-r0)
(6/6) Installing git (2.15.0-r1)
Executing busybox-1.27.2-r7.trigger
Executing ca-certificates-20171114-r0.trigger
OK: 21 MiB in 22 packages
--:
git version 2.15.0

# or much shorter
$ plash-run -A git -- git --version
git version 2.15.0

# If you just want to quickly use some progamm, use plash-use
$ plash-use ag --version
plash-use: caching... (hit Ctrl-d for logs)
ag version 2.1.0

Features:
  +jit +lzma +zlib

# plash-run isolates only certain parts of the filesystem.
# if you want runc (https://github.com/opencontainers/runc) use plash-runc
$ plash-runc --os arch
[root@arch /]# exit

# or shorter
$ plash-runc -R
$ [root@arch /]# exit
```
