import sys
from subprocess import check_output
import subprocess
import sys

inf = open(sys.argv[1])


def run(command, stri):
    print("running:", command, repr(stri))
    try:
        subprocess.run(["plash", command, "/bin/sh", "-cxe", stri], check=True)
    except subprocess.CalledProcessError as e:
        print("ERROR: bad status:", e.returncode)
        sys.exit(1)
    print("ok")


firstline = inf.readline()
assert firstline.startswith("FROM ")
check_output(["plash", "cached", "pull:lxc", "alpine:edge"])

command = "LAYER"
stri = ""

while True:
    line = inf.readline()

    command_begin = line.isupper()
    no_more_lines = not line
    command_ended = no_more_lines or command_begin

    if command_ended:
        run(command, stri)

        if no_more_lines:
            break

    if not command_begin:
        stri += line
    else:
        stri = ""
        command = line[:-1]


