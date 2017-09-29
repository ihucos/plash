import shlex
import uuid
from importlib import import_module

# ECHO_DEBUG = "echo \*\*\* plash: running --{action_name} {args}"
ECHO_DEBUG = "# {action_name}\({args}\)"

layer_marker_rand = str(uuid.uuid4()).split('-')[-1]
state = {'actions': {}}  # put that in state.py ?


class ArgError(TypeError):
    pass


class ActionNotFoundError(Exception):
    pass


class EvalError(Exception):
    pass


def action(action_name=None, echo=True):
    def decorator(func):

        action = action_name or func.__name__.replace('_', '-')

        def function_wrapper(*args, **kw):
            res = func(*args, **kw)

            # allow actions to yield each line
            if not isinstance(res, str) and res is not None:
                res = '\n'.join(res)

            if not echo:
                return res
            else:
                echo_cmd = ECHO_DEBUG.format(
                    action_name=shlex.quote(action),
                    args=', '.join(shlex.quote(shlex.quote(i)) for i in args))
                return "{}\n{}".format(echo_cmd, res or '')

        state['actions'][action] = function_wrapper
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
        action_name = item[0]
        args = item[1:]
        actions = state['actions']
        try:
            action = actions[action_name]
        except KeyError:
            raise ActionNotFoundError(
                'Action "{}" not found'.format(action_name))
        res = action(*args)
        if not isinstance(res, str) and res is not None:
            raise EvalError(
                'eval action must return string or None ({} returned {})'.
                format(action_name, type(res)))
        if res is not None:
            action_values.append(res)
    return '\n'.join(action_values)


@action('original-import', echo=False)
@action('import', echo=False)
def import_planch_actions(*modules):
    output = []
    for module_name in modules:
        import_module(module_name)


@action('original-layer', echo=False)
@action('layer', echo=False)
def layer():
    return ": 'Start new layer marker [{}]'".format(layer_marker_rand)
