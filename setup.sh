#!/bin/sh

set -e

missing=''
for prog in python3 bash make cc
do
  if ! [ -x "$(command -v $prog)" ]; then
    missing="$missing $prog"
  fi
done
if ! [ -z "$missing" ]; then
  echo "error: missing requirement:$missing"
  exit 1
fi

set -x
mkdir -p /opt/plash
curl -Lf https://github.com/ihucos/plash/archive/${1:-master}.tar.gz | tar -xzC /opt/plash --strip-components=1
cd /opt/plash
make

ln -sf /opt/plash/bin/plash      /usr/local/bin/plash
ln -sf /opt/plash/bin/plash-exec /usr/local/bin/plash-exec
