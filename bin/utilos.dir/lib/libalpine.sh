#!/bin/sh

PREFIX=

PROGRAM=$(basename $0)

: ${ROOT:=/}
[ "${ROOT}" = "${ROOT%/}" ] && ROOT="${ROOT}/"
[ "${ROOT}" = "${ROOT#/}" ] && ROOT="${PWD}/${ROOT}"

echon () {
	if [ X"$ECHON" = X ]; then
		# Determine how to "echo" without newline: "echo -n"
		# or "echo ...\c"
		if [ X$(echo -n) = X-n ]; then
			ECHON=echo
			NNL="\c"
			# "
		else
			ECHON="echo -n"
			NNL=""
		fi
	fi
	$ECHON "$*$NNL"
}

# echo if in verbose mode
vecho() {
	[ -n "$VERBOSE" ] && echo "$@"
}

# echo unless quiet mode
qecho() {
	[ -z "$QUIET" ] && echo "$@"
}

# echo to stderr
eecho() {
	echo "$@" >&2
}

# echo to stderr and exit with error
die() {
	eecho "$@"
	exit 1
}

init_tmpdir() {
	local omask=$(umask)
	local __tmpd="/tmp/$PROGRAM-${$}-$(date +%s)-$RANDOM"
	umask 077 || die "umask"
	mkdir -p "$__tmpd" || exit 1
	trap "rm -fr \"$__tmpd\"; exit" 0
	umask $omask
	eval "$1=\"$__tmpd\""
}

pkg_inst() {
	eecho "WARNING: pkg_inst is deprecated. Use 'apk add --quiet' in script"
	[ -z "$NOCOMMIT" ] && apk add --quiet $*
}

pkg_deinst() {
	eecho "WARNING: pkg_deinst is deprecated. Use 'apk del --quiet' in script"
	[ -z "$NOCOMMIT" ] && apk del --quiet $*
}

default_read() {
	local n
	read n
	[ -z "$n" ] && n="$2"
	eval "$1=\"$n\""
}


cfg_add() {
	[ -z "$NOCOMMIT" ] && lbu_add "$@"
}

# return true if given value is 1, Y, Yes, yes YES etc
yesno() {
	case $1 in
	[Yy]|[Yy][Ee][Ss]|1|[Tt][Rr][Uu][Ee]) return 0;;
	esac
	return 1
}

# Detect if we are running Xen
is_xen() {
	test -d /proc/xen
}

# Detect if we are running Xen Dom0
is_xen_dom0() {
	is_xen && \
	grep -q "control_d" /proc/xen/capabilities 2>/dev/null
}

# list of available network interfaces that aren't part of any bridge or bond
available_ifaces() {
	local iflist= ifpath= iface= i=
	if ! [ -d /sys/class/net ]; then
		ip link | awk -F: '$1 ~ /^[0-9]+$/ {printf "%s",$2}'
		return
	fi
	sorted_ifindexes=$(
		for i in /sys/class/net/*/ifindex; do
			[ -e "$i" ] || continue
			echo -e "$(cat $i)\t$i";
		done | sort -n | awk '{print $2}')
	for i in $sorted_ifindexes; do
		ifpath=${i%/*}
		iface=${ifpath##*/}
		# skip interfaces that are part of a bond or bridge
		if [ -d "$ifpath"/master/bonding ] || [ -d "$ifpath"/brport ]; then
			continue
		fi
		iflist="${iflist}${iflist:+ }$iface"
	done
	echo $iflist
}

# from OpenBSD installer

# Ask for a password, saving the input in $resp.
#    Display $1 as the prompt.
#    *Don't* allow the '!' options that ask does.
#    *Don't* echo input.
#    *Don't* interpret "\" as escape character.
askpass() {
	echo -n "$1 "
	set -o noglob
	stty -echo
	read -r resp
	stty echo
	set +o noglob
	echo
}

# Ask for a password twice, saving the input in $_password
askpassword() {
	local _oifs=$IFS
	IFS=
	while :; do
		askpass "Password for $1 account? (will not echo)"
		_password=$resp

		askpass "Password for $1 account? (again)"
		# N.B.: Need quotes around $resp and $_password to preserve leading
		#       or trailing spaces.
		[ "$resp" = "$_password" ] && break

		echo "Passwords do not match, try again."
	done
	IFS=$_oifs
}

# test the first argument against the remaining ones, return success on a match
isin() {
	local _a=$1 _b
	shift
	for _b; do
		[ "$_a" = "$_b" ] && return 0
	done
	return 1
}

# remove all occurrences of first argument from list formed by
# the remaining arguments
rmel() {
	local _a=$1 _b

	shift
	for _b; do
		[ "$_a" != "$_b" ] && echo -n "$_b "
	done
}

# Issue a read into the global variable $resp.
_ask() {
	local _redo=0

	read resp
	case "$resp" in
	!)	echo "Type 'exit' to return to setup."
		sh
		_redo=1
		;;
	!*)	eval "${resp#?}"
		_redo=1
		;;
	esac
	return $_redo
}

# Ask for user input.
#
#    $1    = the question to ask the user
#    $2    = the default answer
#
# Save the user input (or the default) in $resp.
#
# Allow the user to escape to shells ('!') or execute commands
# ('!foo') before entering the input.
ask() {
	local _question=$1 _default=$2

	while :; do
		echo -n "$_question "
		[ -z "$_default" ] || echo -n "[$_default] "
		_ask && : ${resp:=$_default} && break
	done
}

# Ask for user input until a non-empty reply is entered.
#
#    $1    = the question to ask the user
#    $2    = the default answer
#
# Save the user input (or the default) in $resp.
ask_until() {
	resp=
	while [ -z "$resp" ] ; do
		ask "$1" "$2"
	done
}

# Ask for the user to select one value from a list, or 'done'.
#
# $1 = name of the list items (disk, cd, etc.)
# $2 = question to ask
# $3 = list of valid choices
# $4 = default choice, if it is not specified use the first item in $3
#
# N.B.! $3 and $4 will be "expanded" using eval, so be sure to escape them
#       if they contain spooky stuff
#
# At exit $resp holds selected item, or 'done'
ask_which() {
	local _name=$1 _query=$2 _list=$3 _def=$4 _dynlist _dyndef

	while :; do
		# Put both lines in ask prompt, rather than use a
		# separate 'echo' to ensure the entire question is
		# re-ask'ed after a '!' or '!foo' shell escape.
		eval "_dynlist=\"$_list\""
		eval "_dyndef=\"$_def\""

		# Clean away whitespace and determine the default
		set -o noglob
		set -- $_dyndef; _dyndef="$1"
		set -- $_dynlist; _dynlist="$*"
		set +o noglob
		[ $# -lt 1 ] && resp=done && return

		: ${_dyndef:=$1}
		echo "Available ${_name}s are: $_dynlist."
		echo -n "Which one $_query? (or 'done') "
		[ -n "$_dyndef" ] && echo -n "[$_dyndef] "
		_ask || continue
		[ -z "$resp" ] && resp="$_dyndef"

		# Quote $resp to prevent user from confusing isin() by
		# entering something like 'a a'.
		isin "$resp" $_dynlist done && break
		echo "'$resp' is not a valid choice."
	done
}
