set -eu

echo 'run a program on the base layer of a container'
plash.build -o zesty
out=$(plash.run zesty -- echo hellow)
test "$out" == hellow

echo 'create a new layer and check that for programs' \
' the current working directory is mapped'
newcont=$(plash.build -o zesty --bust-cache)
cd /tmp
out=$(plash.run $newcont pwd)
test "$out" == "/tmp"
adf
