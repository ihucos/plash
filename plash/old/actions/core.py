
import hashlib
import os
import shlex
import stat
import subprocess
import sys
from base64 import b64encode

import yaml

import uuid

from .base import actions as ACTIONS
from .base import Action, ArgError


class LAYER:
    pass
LAYER = LAYER()

# should not be here
def tokenize_actions(actions):
    tokens = []
    for action_name, args in actions:
        action = ACTIONS[action_name]
        cmd = action.friendly_call(*args)
        if isinstance(cmd, str) or cmd is LAYER:
            tokens.append(cmd)
        elif isinstance(cmd, list):
            tokens.extend(cmd)
        else:
            raise ValueError('bad action return value')
    return tokens




