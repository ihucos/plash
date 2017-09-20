# make this threadlocal?
# dirty global state seems to be addictive

_state = {}


def set_os(os):
    _state['os'] = os

def get_os():
    return _state.get('os')

# def add_mount(from_, to):
#     _state.setdefault('mountpoints', [])
#     _state['mountpoints'].append((from_, to))

# def pop_mountpoints():
#     return _state.pop('mountpoints', [])

def reset():
    _state.clear()
