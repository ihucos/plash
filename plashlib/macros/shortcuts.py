from plashlib.eval import register_macro

ALIASES = dict(
    x=[['run']],
    l=[['layer']],
    I=[['include']],
    i=[['image']],
    A=[['image', 'alpine'], ['apk']],
    U=[['image', 'ubuntu'], ['apt']],
    F=[['image', 'fedora'], ['dnf']],
    D=[['image', 'debian'], ['apt']],
    C=[['image', 'centos'], ['yum']],
    R=[['image', 'arch'], ['pacman']],
    G=[['image', 'gentoo'], ['emerge']],
)

for name, macro in ALIASES.items():

    def bounder(macro=macro):
        def func(*args):
            # list(args) throws an exception exception for some really funny reason
            # therefore the list comprehension
            args = [i for i in args]
            return eval(macro[:-1] + [macro[-1] + args])

        func.__doc__ = 'alias for: {}[ARG1 [ARG2 [...]]]'.format(' '.join(
            '--' + i[0] + ' ' + ' '.join(i[1:]) for i in macro))
        return func

    func = bounder()
    register_macro(name=name, group='macros')(func)
