#!/bin/bash
set -e

log=$(mktemp)

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
for config in $DIR/test.d/*.sh ; do
  printf $(basename "$config")
  (
  export PS4='+ ${BASH_SOURCE}:${LINENO} '
  [ "$1" == chatty ] || exec > $log 2>&1
  set -x
  source "$config"
  ) &
  if wait $!; then
    echo ' PASS'
  else
    echo ' FAIL'
    tail $log
    echo
  fi

done
