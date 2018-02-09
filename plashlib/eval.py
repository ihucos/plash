import shlex
import uuid
from importlib import import_module
from functools import wraps

LAYER_MARKER = '### start new layer'
state = {'actions': {}}  # put that in state.py ?


class ArgError(TypeError):
    pass


class ActionNotFoundError(Exception):
    pass


class EvalError(Exception):
    pass

def get_actions():
    return state['actions']

def action(name=None, keep_comments=False, escape=True, group=None):
    def decorator(func):

        action = name or func.__name__.replace('_', '-')

        @wraps(func)
        def function_wrapper(*args, **kw):

            if not keep_comments:
                args = [i for i in args if not i.startswith('#')]

            if escape:
                args = [shlex.quote(i) for i in args]

            res = func(*args, **kw)

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
            raise ActionNotFoundError(
                'Action "{}" not found'.format(name))
        res = action(*args)
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
    return LAYER_MARKER
