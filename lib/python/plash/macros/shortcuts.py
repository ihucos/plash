from plash.eval import eval, register_macro

ALIASES = dict(
    x=[["run"]],
    l=[["layer"]],
    f=[["from"]],

    # pin down whatever will last the longest like that in the source
    # code 
    A=[["from", "alpine:edge"], ["apk"]],
    U=[["from", "ubuntu:bionic"], ["apt"]],
    F=[["from", "fedora:31"], ["dnf"]],
    D=[["from", "debian:sid"], ["apt"]],
    C=[["from", "centos:8"], ["yum"]],
    R=[["from", "archlinux:current"], ["pacman"]],
    G=[["from", "gentoo:current"], ["emerge"]],
)

for name, macro in ALIASES.items():

    def bounder(macro=macro):
        def func(*args):
            # list(args) throws an exception exception for some really funny reason
            # therefore the list comprehension
            args = [i for i in args]
            return eval(macro[:-1] + [macro[-1] + args])

        func.__doc__ = "alias for: {}[ARG1 [ARG2 [...]]]".format(
            " ".join("--" + i[0] + " " + " ".join(i[1:]) for i in macro)
        )
        return func

    func = bounder()
    register_macro(name=name, group="macros")(func)
