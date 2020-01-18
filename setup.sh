#!/bin/sh

set -eu

missing=''
for prog in python3 make bash cc curl gpgv
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
curl -Lf https://github.com/ihucos/plash/archive/${1:-master}.tar.gz | tar -xzC / --strip-components=1
cd /opt/plash
make