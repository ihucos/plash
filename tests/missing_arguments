#/bin/bash
set -eu

array=(
plash
plash.build
plash.do
plash.mount
plash.nodepath
plash.rm
plash.run
plash.runfile
plash.runp
plash.tarout
)
for prog in "${array[@]}"
do
  out=$($prog 2>&1) || true
  echo "$out" | grep '/bin/plash' && {
    echo "No arguments seems to crash $prog"
    exit 1
  }
done
