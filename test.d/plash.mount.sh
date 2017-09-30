#!/bin/bash
set -eu

plash.build -o zesty >/dev/null
TMP=$(mktemp -d)
plash.mount zesty $TMP
grep  $TMP' '  /proc/mounts >/dev/null || {
  echo 'Could not find mountpoint in /proc/mounts '
  exit 1
}
echo hi
test -d $TMP/usr
test -d $TMP/bin
test -d $TMP/var
