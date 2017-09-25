# make this threadlocal?
# dirty global state seems to be addictive

_state = {}


def set_os(os):
    _state['os'] = os

def get_os():
    return _state.get('os')

def add_volume(name, path):
    _state.setdefault('volumes', [])
    _state['volumes'].append((name, path))

def pop_volumes():
    return _state.pop('volumes', [])

def reset():
    _state.clear()
