from plash.eval import eval, register_macro

ALIASES = dict(
    x=[["run"]],
    l=[["layer"]],
    f=[["from"]],

    # Update on an best effort basis. If possible keep versions that point to a
    # latest release.
    A=[["from", "alpine:edge"], ["apk"]],
    U=[["from", "ubuntu:focal"], ["apt"]],
    F=[["from", "fedora:35"], ["dnf"]],
    D=[["from", "debian:bullseye"], ["apt"]],
    C=[["from", "centos:8"], ["yum"]],
    R=[["from", "archlinux:current"], ["pacman"]],
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
