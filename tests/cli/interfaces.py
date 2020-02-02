#!/usr/env/bin python3

import subprocess
import tempfile
from pprint import pprint
import os

def run(*cmd):
    p = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    child_stdout = p.stdout.decode()
    child_stderr = p.stderr.decode()
    child_exit = p.returncode
    return {
        'stdout': child_stdout,
        'stderr': child_stderr,
        'exit': child_exit,
    }

res = run('plash', 'help')
assert res['exit'] == 0
all_plash_commands = set(line.split()[1] for line in res['stdout'].splitlines())
all_plash_commands.remove('test')

def runs_with_no_arguments(plash_cmd):
    res = run("plash", plash_cmd)
    assert res['exit'] == 0

def usage_if_no_arguments(plash_cmd):
    res = run("plash", plash_cmd)
    assert res['stdout'] == ''
    assert res['stderr'].startswith("usage: plash {}".format(plash_cmd))
    assert res['exit'] == 1

def usage_if_one_argument(plash_cmd):
    res = run("plash", plash_cmd, "token")
    assert res['stdout'] == ''
    assert res['stderr'].startswith("usage: plash {}".format(plash_cmd))
    assert res['exit'] == 1

def complains_if_first_arg_not_container(plash_cmd):
    res = run("plash", plash_cmd, "token")
    assert res['stderr'] == "plash error: container arg must be a positive number, got: token\n"
    assert res['stdout'] == ""
    assert res['exit'] == 1

def exports(plash_cmd):
    tmpdir = tempfile.mkdtemp()
    out = os.path.join(tmpdir, 'out')
    res = run("plash", plash_cmd, '1', out)
    if plash_cmd == 'export-tar':
        print(res)
    assert os.path.exists(out)
    assert res['exit'] == 0

testfuncs = [
    exports,
    #complains_if_first_arg_not_container,
    #runs_with_no_arguments,
    #usage_if_no_arguments,
]

os.close(0)
for testfunc in testfuncs:
    for plash_cmd in all_plash_commands:
        try:
            testfunc(plash_cmd)
        except AssertionError as exc:
            continue
        print(testfunc.__name__, plash_cmd)
    print()

