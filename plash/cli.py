import argparse
import os
import subprocess
import sys
from subprocess import CalledProcessError

from .eval import ActionNotFoundError, ArgError, EvalError, eval, layer
from .runos import BuildError, LayeredDockerBuildable, docker_run, runos
from .utils import (disable_friendly_exception, friendly_exception, hashstr,
                    rand)

HELP = 'my help'
PROG = 'plash'

SHORTCUTS = [
    # shortcut, lsp, nargs
    ('-a', ['apt'], '+'),
    ('-p', ['pip'], '+'),
    ('-b', ['apt', 'ubuntu-server'], 0),
 ]


def add_shortcuts_to_parser(parser):
    group = parser.add_argument_group('shortcuts', 'shortcuts')
    for shortcut, lsp, nargs in SHORTCUTS:
        group.add_argument(
            shortcut,
            action=create_collect_lsp_action(lsp),
            nargs=nargs)

def create_collect_lsp_action(lsp_begin):
    class CollectAction(argparse.Action):
        def __call__(self, parser, namespace, values, option_string=None):
            if not 'lsp' in namespace:
                setattr(namespace, 'lsp', [])
            previous = namespace.lsp
            previous.append(lsp_begin + list(values))
            setattr(namespace, 'lsp', previous) 
    return CollectAction

# def unused_args_to_lsp(args):
#     lsp = []
#     for token in args:
#         if token.startswith('-'):
#             lsp.append([token.lstrip('-')])
#         else:
#             lsp[-1].append(token)
#     return lsp


def get_argument_parser():
    parser = argparse.ArgumentParser(
        formatter_class=argparse.RawTextHelpFormatter,
        description='Run programm from any Linux',
        prog=PROG,
        epilog=HELP)

    parser.add_argument("--build-quiet", action='store_true', dest='quiet')
    parser.add_argument("--build-verbose", "--build-loud", action='store_true', dest='verbose')
    parser.add_argument("--build-only", action='store_true')
    parser.add_argument("--build-again", "--rebuild", "--again", action='store_true')
    parser.add_argument("--no-stdlib", action='store_true')
    parser.add_argument("--traceback", action='store_true')
    parser.add_argument("--debug-lsp", action='store_true')

    parser.add_argument("--save-image")

    parser.add_argument(
        "image", type=str)
    parser.add_argument(
        "exec", type=str, nargs='*', default=['bash'])

    add_shortcuts_to_parser(parser)

    return parser



def main():
    ap = get_argument_parser()
    _, unused_args = ap.parse_known_args(sys.argv[1:])
    # lsp = unused_args_to_lsp(unused_args)
    for arg in unused_args:
        if arg == '--':
            break
        if arg.startswith('--'):
            ap.add_argument(
                arg,
                action=create_collect_lsp_action([arg[2:]]),
                nargs='*')
    args = ap.parse_args()
    lsp = getattr(args, 'lsp', [])

    if args.traceback:
        disable_friendly_exception()
    if not args.no_stdlib:
        init = [['import', 'plash.stdlib']]
    else:
        init = []

    if args.debug_lsp:
        print(init + lsp)
        sys.exit(0)
    with friendly_exception([ActionNotFoundError, ArgError, EvalError]):
        script = eval(init + lsp)

    layers = script.split('{}'.format(layer()))
    plash_env = '{}-{}'.format(
        args.image,
        hashstr('\n'.join(layers).encode())[:4])
    if args.image == 'print':
        print(script)
        sys.exit(0)


    if args.image.startswith('build://'):
        build = args.image[len('build://'):]
        tmp_image = rand() # fixme cleanup this image later
        p = subprocess.Popen(['docker', 'build', build, '-t', tmp_image])
        exit = p.wait()
        assert exit == 0
        image = subprocess.check_output(
            ['docker', 'images', '--quiet', tmp_image])
        image = image.decode().rstrip('\n')
        subprocess.check_output(['docker', 'rmi', tmp_image])
    else:
        image = args.image

    b = LayeredDockerBuildable.create(image, layers)

    with friendly_exception([BuildError, CalledProcessError]):
        if args.build_again:
            b.build(
                quiet=args.quiet,
                verbose=args.verbose)
        else:
            b.ensure_builded(
                quiet=args.quiet,
                verbose=args.verbose)

    if args.save_image:
        with friendly_exception([CalledProcessError], 'save-image'):
            container_id = subprocess.check_output(
                ['docker', 'run', b.get_image_name(), 'hostname'])
            container_id = container_id.decode().strip('\n')
            subprocess.check_output(
                ['docker', 'commit', container_id, args.save_image])

    command = args.exec if not args.build_only else None
    if command:
        exit = docker_run(b.get_image_name(), command,
                          extra_envs={'PLASH_ENV': plash_env})
        sys.exit(exit)
