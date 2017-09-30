#!/bin/bash
set -eu
! plash.nodepath mynoexistentcontainer
newcont=$(plash.build -o zesty --bust-cache)
nodepath=$(plash.nodepath $newcont)
test $nodepath == /var/lib/plash/builds/zesty/children/$newcont
layerup=$(plash.build -o $newcont --layer --bust-cache)
nodepath=$(plash.nodepath $layerup)
test $nodepath == /var/lib/plash/builds/zesty/children/$newcont/children/$layerup
