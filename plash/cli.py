import os
import sys


def main():
    argv = sys.argv[1:]
    subcommand = argv[0]
    subcommands_dir = os.path.join(
        os.path.abspath(os.path.dirname(__file__)),
        'subcommands')
    all_subcommands = os.listdir(subcommands_dir)
    if not subcommand in all_subcommands:
        with friendly_exception([ArgError]):
            raise ArgError('no subcommand {}, available: {}'.format(repr(subcommand), ' '.join(all_subcommands)))

    subcommand_executable = os.path.join(subcommands_dir, subcommand)
    os.execvpe(
        subcommand_executable,
        [subcommand_executable] + argv[1:],
        dict())
