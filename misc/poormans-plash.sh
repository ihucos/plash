#!/bin/sh

set -eux

BINPATH="/usr/local/bin"
URL='http://dl-cdn.alpinelinux.org/alpine/v3.8/releases/x86_64/alpine-minirootfs-3.8.1-x86_64.tar.gz'
APPDIR=/opt/plash
ROOTFS="$APPDIR"/rootfs
ROOTFSMNT="$APPDIR"/rootfsmnt
ALPINEPATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin

mkdir -p "$ROOTFS"
mkdir -p "$ROOTFSMNT"

# doownload the root file system
curl "$URL" | tar -xz -C "$ROOTFS"

# assert it exists and is not a link for the mount on it to work
# then copy the one from the host
rm "$ROOTFS"/etc/resolv.conf || true
cp /etc/resolv.conf "$ROOTFS"/etc/resolv.conf
    
# install our stuff there, pip3 needs /dev/urandom in some cases
mount --rbind /dev "$ROOTFSMNT"/dev
chroot "$ROOTFS" env PATH="$ALPINEPATH" apk update
chroot "$ROOTFS" env PATH="$ALPINEPATH" apk add py3-pip bash
chroot "$ROOTFS" env PATH="$ALPINEPATH" pip3 install plash
umount "$ROOTFSMNT"/dev

# write a script that creates our mounts if necessary
# and the runs the chrooted plash
cat > "$BINPATH"/plash <<- PlashBootstrapScriptEOF

# plash runs an equivalent of `mount --make-rprivate /`
# for this to work / must be a mountpoint, this is why we also get this mountpoint.
mountpoint "$ROOTFSMNT"                 > /dev/null || mount --bind "$ROOTFS"         "$ROOTFSMNT"

# mount to the chrooted os so plash can forward it to its containers
mountpoint "$ROOTFSMNT"/tmp             > /dev/null || mount --rbind /tmp             "$ROOTFSMNT"/tmp
mountpoint "$ROOTFSMNT"/home            > /dev/null || mount --rbind /home            "$ROOTFSMNT"/home
mountpoint "$ROOTFSMNT"/root            > /dev/null || mount --rbind /root            "$ROOTFSMNT"/root
mountpoint "$ROOTFSMNT"/sys             > /dev/null || mount --rbind /sys             "$ROOTFSMNT"/sys
mountpoint "$ROOTFSMNT"/dev             > /dev/null || mount --rbind /dev             "$ROOTFSMNT"/dev
mountpoint "$ROOTFSMNT"/proc            > /dev/null || mount --rbind /proc            "$ROOTFSMNT"/proc

# find out what to exec (plash or plash-exec) and exec it!
prog=\$(basename "\$0")
exec chroot "$ROOTFSMNT" env PATH="$ALPINEPATH" "\$prog" "\$@"
PlashBootstrapScriptEOF

chmod +x "$BINPATH"/plash
ln -fs "$BINPATH"/plash "$BINPATH"/plash-exec
