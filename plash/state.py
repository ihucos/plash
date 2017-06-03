# make this threadlocal?

_state = {}


def set_os(os):
    _state['os'] = os

def get_os():
    return _state.get('os')

def set_base_command(cmd):
    _state['base-command'] = cmd

def get_base_command():
    return _state.get('base-command')

def reset():
    _state.clear()
