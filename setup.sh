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
mkdir - /opt/plash
curl -Lf https://github.com/ihucos/plash/archive/${1:-master}.tar.gz | tar -xz --strip-components=1
make
