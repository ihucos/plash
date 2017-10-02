#!/bin/bash
set -eu
test "$(plash -o zesty)" == zesty
out=$(plash -o zesty -- printf hello)
test $out == hello
test $(plash nodepath zesty) == $(plash.nodepath zesty)
