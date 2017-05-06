import shlex

from utils import friendly_exception


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
        with self.friendly_exception(
            self.base_friendly_exceptions + self.friendly_exceptions):
            debug = "echo \*\*\* plash is running --{} {}".format(
                shlex.quote(self.name), ' '.join(shlex.quote(i) for i in args))
            # return debug + ' && ' + self(*args)
            return self(*args)

    def friendly_exception(self, exceptions):
        return friendly_exception(exceptions, self.name)
