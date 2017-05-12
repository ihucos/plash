import shlex

from ..utils import friendly_exception


class ArgError(Exception):
    pass

actions = {}
class ActionMeta(type):
    def __new__(cls, clsname, superclasses, attributedict):
        cls = type.__new__(cls, clsname, superclasses, attributedict)
        for sp in superclasses:
            o = cls()
            actions[o.name] = o
        return cls

class Action(metaclass=ActionMeta):

    base_friendly_exceptions = [IOError, ArgError]
    friendly_exceptions = []

    @property
    def name(self):
        return self.__class__.__name__.lower()

    @classmethod
    def call(cls, *args):
        return cls()(*args)

    def friendly_call(self, *args, **kwargs):
        from .core import LAYER
        with self.friendly_exception(
            self.base_friendly_exceptions + self.friendly_exceptions):
            debug = "echo \*\*\* plash is running --{} {}".format(
                shlex.quote(self.name), ' '.join(shlex.quote(i) for i in args))
            cmd = self(*args)
            if isinstance(cmd, str):
                return [debug, cmd]
            elif isinstance(cmd, list):
                return [debug] + cmd
            elif cmd is LAYER:
                return [debug, cmd]

    def friendly_exception(self, exceptions):
        return friendly_exception(exceptions, self.name)
