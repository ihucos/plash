import shlex

from .eval import (ActionNotFoundError, ArgError, EvalError
                   register_action)
from .utils import friendly_exception


def action(action_name, echo=True):
    def decorator(func):
        @register_action(action_name)
        def wrapper(*args, **kw):
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
        return wrapper
    return decorator
