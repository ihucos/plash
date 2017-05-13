import hashlib
import os
import stat
import sys
import uuid
from contextlib import contextmanager


def rand():
    return str(uuid.uuid4()).split('-')[-1]

def hashstr(stri):
    return hashlib.sha1(stri).hexdigest()

friendly_exception_sate = {'enabled': True}
def disable_friendly_exception():
    friendly_exception_sate['enabled'] = False

@contextmanager
def friendly_exception(exceptions, debug=None):
    try:
        yield
    except tuple(exceptions) as exc:
        if not friendly_exception_sate['enabled']:
            raise exc
        else:
            if debug:
                msg = 'plash error at {debug}: {message}'.format(
                    debug=debug,
                    message=str(exc))
            else:
                msg = 'plash error: {message}'.format(message=str(exc))

            print("\033[91m{}\033[0m".format(msg))
            # raise exc # TODO: experiment with nested friendly_exception for example at load
            sys.exit(1)

# def create_executable_file(fname, script):
#     if os.path.exists(fname):
#         raise SystemExit('File {} already exists - deal with this'.format(fname))

#     with open(fname, 'w') as f:
#         f.write(script)
#     st = os.stat(fname)
#     os.chmod(fname, st.st_mode | stat.S_IEXEC)
