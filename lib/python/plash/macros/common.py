import os
import sys

from plash.eval import eval, hint, join_result, register_macro, shell_escape_args
from plash import utils


@register_macro()
def layer(command=None, *args):
    "hints the start of a new layer"
    if not command:
        return eval([["hint", "layer"]])  # fall back to buildin layer macro
    else:
        lst = [["layer"]]
        for arg in args:
            lst.append([command, arg])
            lst.append(["layer"])
        return eval(lst)


@register_macro()
def run(*cmds):
    "directly emit shell script"
    return "\n".join(cmds)


@register_macro()
def inline(*cmds):
    "emit all arguments in one line"
    return " ".join(cmds)


@register_macro()
@shell_escape_args
@join_result
def import_env(*envs):
    "import environment variables from host while building"
    import shlex

    for env in envs:
        parts = env.split(":", 1)
        if len(parts) == 1:
            export_as = env
        else:
            env, export_as = parts
        env_val = os.environ.get(env)
        if env_val is not None:
            yield "{}={}".format(export_as, shlex.quote(env_val))


@register_macro()
def invalidate_layer():
    "invalidate the cache of the current layer"
    import uuid

    return ": invalidate cache with {}".format(uuid.uuid4())


@register_macro()
@shell_escape_args
@join_result
def write_file(fname, *lines):
    "write lines to a file"
    yield "touch {}".format(fname)
    for line in lines:
        yield "echo {} >> {}".format(line, fname)

@register_macro()
@shell_escape_args
@join_result
def env(*envs):
    "Use env from host when running image"
    for env in envs:
        yield "echo {} >> /.plashenvs".format(env)

@register_macro()
@shell_escape_args
@join_result
def env_prefix(*prefixes):
    "Use all envs with given prefix when running image"
    for prefix in prefixes:
        yield "echo {} >> /.plashenvsprefix".format(prefix)

@register_macro()
@shell_escape_args
@join_result
def mount(*mounts):
    'Mount filesystem when running image ("src:dst" supported)'
    for prefix in mounts:
        yield "echo {} >> /.plashmounts".format(prefix)

@register_macro()
@join_result
def write_script(fname, *lines):
    "write an executable (755) file to the filesystem"
    yield write_file(fname, *lines)
    yield "chmod 755 {}".format(fname)


@register_macro()
def eval_file(file):
    "evaluate file content as expressions"

    fname = os.path.realpath(os.path.expanduser(file))
    with open(fname) as f:
        inscript = f.read()

    sh = utils.plash_call("eval", "--eval-stdin", strip=False, input=inscript)

    # we remove an possibly existing newline
    # because else this macros would add one
    if sh.endswith("\n"):
        return sh[:-1]

    return sh


@register_macro()
def eval_string(stri):
    "evaluate expressions passed as string"
    import shlex

    tokens = shlex.split(stri)
    return utils.plash_call("eval", *tokens, strip=False)


@register_macro()
def eval_stdin():
    "evaluate expressions read from stdin"
    tokens = [i.rstrip("\n") for i in sys.stdin.readlines()]
    return utils.plash_call("eval", *tokens, strip=False)


@register_macro()
def run_stdin():
    "run commands read from stdin"
    return sys.stdin.read()


@register_macro()
def eval_github(user_repo_pair, file="plashfile"):
    "eval a file (default 'plashfile') from a github repo"
    from urllib.request import urlopen

    url = "https://raw.githubusercontent.com/{}/master/{}".format(user_repo_pair, file)
    with utils.catch_and_die([Exception], debug=url):
        resp = urlopen(url)
    plashstr = resp.read().decode()
    return utils.plash_call("eval", "--eval-stdin", input=plashstr)


class HashPaths:
    "recursively hash files and add as cache key"

    def _list_all_files(self, dir):
        for (dirpath, dirnames, filenames) in os.walk(dir):
            for filename in filenames:
                fname = os.sep.join([dirpath, filename])
                yield fname

    def __call__(self, *paths):
        import hashlib
        import stat

        collect_files = []
        for path in paths:
            if os.path.isdir(path):
                collect_files.extend(self._list_all_files(path))
            else:
                collect_files.append(path)

        hasher = hashlib.sha512()
        for fname in sorted(collect_files):
            perm = str(oct(stat.S_IMODE(os.lstat(fname).st_mode))).encode()
            with open(fname, "rb") as f:
                fread = f.read()  # well, no buffering?
            hasher.update(fname.encode())
            hasher.update(perm)

            hasher.update(utils.hashstr(fread).encode())

        hash = hasher.hexdigest()
        return ": hash: {}".format(hash)


register_macro("hash-path")(HashPaths())


@register_macro()
def entrypoint(exec_path):
    "hint default command for this build"
    return hint("exec", exec_path)


@register_macro()
def entrypoint_script(*lines):
    "write lines to /entrypoint and hint it as default command"
    lines = list(lines)
    if lines and not lines[0].startswith("#!"):
        lines.insert(0, "#!/bin/sh")
    return eval(
        [["entrypoint", "/entrypoint"], ["write-script", "/entrypoint"] + lines]
    )
