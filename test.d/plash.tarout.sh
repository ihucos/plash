#!/bin/bash
tmp=$(mktemp)
plash.do tarout -o edge > $tmp
tar --list -f $tmp >/dev/null
