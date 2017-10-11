set -eu
newcont=$(plash.build -o zesty --bust-cache)
plash.nodepath $newcont
plash.rm $newcont
! plash.nodepath $newcont
