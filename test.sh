#!/usr/bin/env plash
#set -e


# error_handler() {
#   echo "Error: ($1) occurred on $2"
# }
# trap 'error_handler' err



#
# function _trap_debug() {
#     echo ": $BASH_COMMAND";
#   }
# trap '_trap_debug' debug



RUN apk add cowsay
VERIFY cat requirements.txt
RUN pip3 install -r requirements.txt


ENTRYPOINT() {
  exec blah '"$@"'
}
