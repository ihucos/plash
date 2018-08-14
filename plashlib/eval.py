import sys
import re
import os
import shlex
import importlib
from functools import wraps

FIND_HIND_HINT_VALUES_RE = re.compile(
        '### plash hint: ([^=]+)=(.+)\n')

state = {'macros': {}}

class MacroNotFoundError(Exception):
    pass

class MacroError(Exception):
    def __str__(self):
        func, macro_name, (type, value, traceback) = self.args
        return '{macro_module}: {macro_name}: {value} ({type})'.format(
            macro_module=func.__module__,
            macro_name=macro_name,
            value=value,
            type=type.__name__)

class EvalError(Exception):
    pass

def get_macros():
    return state['macros']

def register_macro(name=None, group='main'):
    def decorator(func):
        macro = name or func.__name__.replace('_', '-')
        state['macros'][macro] = func
        return func

    return decorator

def shell_escape_args(func):
    @wraps(func)
    def function_wrapper(*args):
        args = [shlex.quote(i) for i in args]
        return func(*args)
    return function_wrapper

def join_result(func):
    @wraps(func)
    def function_wrapper(*args):
        res = func(*args)
        return '\n'.join(res)
    return function_wrapper

def eval(lisp):
    '''
    plash lisp is one dimensional lisp.
    '''
    macro_values = []
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
        try:
            macro = state['macros'][name]
        except KeyError:
            raise MacroNotFoundError("macro {} not found".format(repr(name)))
        try:
            res = macro(*args)
        except Exception as exc:
            if os.getenv('PLASH_DEBUG_MACROS', '').lower() in ('1', 'yes', 'true'):
                raise
            if isinstance(exc, MacroError):
                # only raise that one time and don't have multiple wrapped MacroError
                raise
            raise MacroError(macro, name, sys.exc_info())
        if not isinstance(res, str) and res is not None:
            raise EvalError(
                'eval macro must return string or None ({} returned {})'.
                format(name, type(res)))
        if res is not None:
            macro_values.append(res)
    return '\n'.join(macro_values)

def get_hint_values(script):
    return dict(FIND_HIND_HINT_VALUES_RE.findall(script))

@register_macro('import')
def import_(*modules):
    output = []
    for module_name in modules:
        importlib.import_module(module_name)

@register_macro()
def reset_imports():
    state['macros'] = {
        'import': import_,
        'hint': hint,
        'reset_imports': reset_imports
    }

@register_macro()
def hint(name, value=None):
    'hint something'
    if value is None:
        return '### plash hint: {}'.format(name)
    else:
        return '### plash hint: {}={}'.format(name, value)
