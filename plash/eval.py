import shlex
from importlib import import_module

from .utils import friendly_exception, rand

layer_marker_rand = rand()
state = {'actions': {}}

class ArgError(TypeError):
        pass

class ActionNotFoundError(Exception):
    pass

class EvalError(Exception):
    pass


def register_action(action_name):
    def decorator(f):
        def function_wrapper(*args, **kw):
            with friendly_exception(
                [ActionNotFoundError, ArgError, EvalError, IOError],
                    action_name):
                res = func(*args, **kw)

                # allow actions to yield each line
                if not isinstance(res, str) and not res is None:
                    res = '\n'.join(res)

            if not echo:
                return res
            else:
                echo_cmd = "echo \*\*\* plash is running :{} {}".format(
                    shlex.quote(action_name), ' '.join(shlex.quote(i) for i in args))
                return "{}\n{}".format(echo_cmd, res)
        return function_wrapper


        state['actions'][action_name] = f
        return f
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
            raise EvalError('must evaluate list of list of strings. not a list of strings: {}'.format(item))
        action_name = item[0]
        args = item[1:]
        actions = state['actions']
        try:
            action = actions[action_name]
        except KeyError:
            raise ActionNotFoundError('Action "{}" not found'.format(
                action_name))
        res = action(*args)
        if not isinstance(res, str) and res is not None:
            raise EvalError('eval action must return string or None ({} returned {})'.format(
                action_name, type(res)))
        if res is not None:
            action_values.append(res)
    return '\n'.join(action_values)

@register_action('original-import')
@register_action('import')
def import_planch_actions(*modules):
    output = []
    for module_name in modules:
        debug = 'importing {}'.format(module_name)
        with friendly_exception([ImportError], debug):
            import_module(module_name)

@register_action('original-layer')
@register_action('layer')
def layer():
    return ": 'Start new layer marker [{}]'".format(layer_marker_rand)

