import shlex
from importlib import import_module

from plash.utils import friendly_exception

# LAYER = object()

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
            raise EvalError('must evaluate list of list of strings')
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
    
    # # split action_values into layers usint LAYER as marker
    # layers = [[]]
    # while action_values:
    #     elem = action_values.pop(0)
    #     if not elem is LAYER:
    #         layers[-1].append(elem)
    #     else:
    #         layers.append([])

    # return layers
    # return '\n'.join(bash_script)

# @register_action('layer')
# def layer():
#     return LAYER

@register_action('import')
def import_planch_actions(*modules):
    output = []
    for module_name in modules:
        debug = 'importing {}'.format(module_name)
        with friendly_exception([ImportError], debug):
            import_module(module_name)
        return ':' # FIXME: return nothing?
        # module = import_module(module_name)
        # for attr in dir(module):
        #     obj = getattr(module, attr)
        #     planch_name = getattr(obj, 'planch_name', None)
        #     if planch_name:
        #         if not callable(obj):
        #             raise TypeError('{} is registered as planch action but is not callable'.format(planch_name))
        #         state['actions'][planch_name] = obj
# state['actions']['import'] = import_planch_actions

# ========== OTHER FILE MAYBE ==========

