import sys
import re
import shlex
from importlib import import_module
from functools import wraps

FIND_HIND_HINT_VALUES_RE = re.compile(
        '### plash hint: ([^=]+)=(.+)\n')

state = {'actions': {}}  # put that in state.py ?


class ActionNotFoundError(Exception):
    pass

class ActionTracebackError(Exception):
    def __str__(self):
        func, action_name, (type, value, traceback) = self.args
        return '{action_module}: {action_name}: {value} ({type})'.format(
            action_module=func.__module__,
            action_name=action_name,
            value=value,
            type=type.__name__,
            )

class EvalError(Exception):
    pass


def get_actions():
    return state['actions']


def action(name=None, keep_comments=False, escape=True, group=None):
    def decorator(func):

        action = name or func.__name__.replace('_', '-')

        @wraps(func)
        def function_wrapper(*args):

            if not keep_comments:
                args = [i for i in args if not i.startswith('#')]

            if escape:
                args = [shlex.quote(i) for i in args]

            res = func(*args)

            # allow actions to yield each line
            if not isinstance(res, str) and res is not None:
                res = '\n'.join(res)

            return res

        state['actions'][action] = function_wrapper
        function_wrapper._plash_group = group
        return function_wrapper

    return decorator


def eval(lisp):
    '''
    plash lisp is one dimensional lisp.
    '''
    action_values = []
    if not isinstance(lisp, list):
        raise EvalError('eval root element must be a list')
    for item in lisp:
        if not isinstance(item, list):
            raise EvalError('must evaluate list of list')
        if not all(isinstance(i, str) for i in item):
            raise EvalError(
                'must evaluate list of list of strings. not a list of strings: {}'.
                format(item))
        name = item[0]
        args = item[1:]
        actions = state['actions']
        try:
            action = actions[name]
        except KeyError:
            raise ActionNotFoundError("action {} not found".format(repr(name)))
        try:
            res = action(*args)
        except Exception as exc:
            if isinstance(exc, ActionTracebackError):
                # only raise that one time and don't have wrapper ActionTracebackError
                raise
            raise ActionTracebackError(action, name, sys.exc_info())
        if not isinstance(res, str) and res is not None:
            raise EvalError(
                'eval action must return string or None ({} returned {})'.
                format(name, type(res)))
        if res is not None:
            action_values.append(res)
    return '\n'.join(action_values)


@action('original-import')
@action('import')
def import_plash_actions(*modules):
    output = []
    for module_name in modules:
        import_module(module_name)


@action('original-layer')
@action('layer')
def layer():
    'start a new layer'
    return hint('layer')

@action('original-hint')
@action('hint')
def hint(name, value=None):
    'hint something'
    if value is None:
        return '### plash hint: {}'.format(name)
    else:
        return '### plash hint: {}={}'.format(name, value)

def get_hint_values(script):
    return dict(FIND_HIND_HINT_VALUES_RE.findall(script))
