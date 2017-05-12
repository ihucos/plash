import shlex
from importlib import import_module

from plash.utils import friendly_exception

LAYER = object()

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
        # if not isinstance(lisp, list):
        #     raise EvalError('eval action must return string ({} returned {})'.format(
        #         action_name, type(res)))
        action_values.append(res)
    
    # split action_values into layers usint LAYER as marker
    layers = [[]]
    while action_values:
        elem = action_values.pop(0)
        if not elem is LAYER:
            layers[-1].append(elem)
        else:
            layers.append([])

    return layers
    # return '\n'.join(bash_script)

@register_action('layer')
def layer():
    return LAYER

@register_action('import')
def import_planch_actions(modules):
    for module_name in modules:
        import_module(module_name)
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

class ActionMeta(type):
    def __call__(cls, *args, **kwargs):
        obj = type.__call__(cls)
        return obj.assisted_call(*args, **kwargs)

    def __new__(cls, clsname, superclasses, attributedict):
        # abstract = attributedict.pop('abstract', None)
        cls = type.__new__(cls, clsname, superclasses, attributedict)
        obj = type.__call__(cls)
        # cls.plash_action = obj.name
        register_action(obj.name)(obj.assisted_call)
        return cls


class Action(metaclass=ActionMeta):

    base_friendly_exceptions = [IOError, ArgError, ActionNotFoundError]
    friendly_exceptions = []
    abstract = False

    @property
    def name(self):
        return self.__class__.__name__.lower()

    def __call__(self, *args):
        if not args:
            raise ArgError('Needs at least one argument')
        stmts = []
        for arg in args:
            stmts.append(self.handle_arg(arg))
        return ' && '.join(stmts)

    def handle_arg(self, arg):
        raise NotImplementedError('implement me or __call__')

    def assisted_call(self, *args):
        with self.friendly_exception(
            self.base_friendly_exceptions + self.friendly_exceptions):
            debug_echo = "echo \*\*\* plash is running --{} {}".format(
                shlex.quote(self.name), ' '.join(shlex.quote(i) for i in args))
            line = self(*args)
            return '{} && {}'.format(debug_echo, line)

    def eval(self, lisp):
        # i can imagine that in the future we use state from this class
        return eval(lisp)

    def friendly_exception(self, exceptions):
        return friendly_exception(exceptions, self.name)


# ========== OTHER FILE ==========

class Include(Action):
    def handle_arg(self, file):
        fname = os.path.realpath(fname)
        with open(fname) as f:
            config = f.read()
        loaded = yaml.load(config)
        return self.eval(loaded)


class LayeEach(Action):
    name = 'layer-each'

    def __call__(self, command, *args):
        lst = []
        for arg in args:
            lst.append([command, arg])
            lst.append(['layer'])
        return self.eval(lst)

class Inline(Action):
    name = 'inline'
    def handle_arg(self, arg):
        return arg

# class Home(Action):

#     def __call__(self):
#         return self.eval([['include', path.join(home_path, '.plash.yaml')]])


print(eval([['layer-each', 'inline', 'hi', 'ho']]))
