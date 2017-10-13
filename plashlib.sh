set -eu

USAGE_ERROR=2

checkargs(){
  [ $# -eq $1 ] && {
    echo "USAGE: $(basename $0) $2" >&2
      exit 2
  }
}
