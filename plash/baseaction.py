import shlex

from .eval import ActionNotFoundError, ArgError, eval, register_action
from .utils import friendly_exception


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
