# make this threadlocal?
# dirty global state seems to be addictive

_state = {}


def set_os(os):
    _state['os'] = os

def get_os():
    return _state.get('os')

def set_base_command(cmd):
    _state['base-command'] = cmd

def get_base_command():
    return _state.get('base-command')

def add_mount(from_, to, *, readonly):
    _state.setdefault('mountpoints', [])
    _state['mountpoints'].append((from_, to, readonly))

def pop_mountpoints():
    return _state.pop('mountpoints', [])

def reset():
    _state.clear()
