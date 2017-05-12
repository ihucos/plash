state = {'actions': {}}

class ArgError(TypeError):
        pass

class ActionNotFoundError(Exception):
    pass

class EvalError(Exception):
    pass


def eval_lisp(lisp):
    '''
    plash lisp is one domensional lisp.
    '''
    bash_script = []
    if not isinstance(lisp, list):
        raise EvalError('eval root element must be a list')
    for item in lisp:
        if not isinstance(item, list):
            raise EvalError('must evaluate list of list')
        if not all(isinstance(item, str)):
            raise EvalError('must evaluate list of list of strings')
        action_name = item[0]
        args = item[1:]
        actions = state['actions']
        try:
            action = actions[action_name]
        except KeyError:
            raise ActionNotFoundError('action {} not found'.format(
                action_name))
        res = action(*args)
        if not isinstance(lisp, list):
            raise EvalError('eval action must return string ({} returned {})'.format(
                action_name, type(res)))
        bash_script.append(res)
    
    return '\n'.join(bash_script)


  


class CollectActionsMeta(type):
    def __new__(cls, clsname, superclasses, attributedict):
        cls = type.__new__(cls, clsname, superclasses, attributedict)
        o = cls()
        if not o.abstract:
            state['actions'][o.name] = o.assisted_call
        attributedict.pop('abstract', None)
        return super().__new__(cls, clsname, superclasses, attributedict)


class Action(metaclass=CollectActionMeta):

    base_friendly_exceptions = [IOError, ArgError]
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
                shlex.quote(self.name), ' '.join(shlex.quote(args)))
            line = self(*args)
            return ' && '.format(debug_echo, line)

    def eval(self, lisp):
        return eval_lisp(lisp)

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
