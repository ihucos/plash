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
        # f.plash_action = action_name
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
            raise ActionNotFoundError('Action {} not found'.format(
                action_name))
        res = action(*args)
        if not isinstance(res, str):
            raise EvalError('eval action must return string ({} returned {})'.format(
                action_name, type(res)))
        action_values.append(res)
    return '\n'.join(action_values)

@register_action('import')
def import_planch_actions(*modules):
    output = []
    for module_name in modules:
        debug = 'importing {}'.format(module_name)
        with friendly_exception([ImportError], debug):
            import_module(module_name)
        return ':' # FIXME: return nothing?

@register_action('layer')
def layer():
    return ": 'Start new layer marker [{}]'".format(layer_marker_rand)

