import coverage
coverage.process_startup()

import os

orig = os.execlp


def patched_execlp(*args):
    from coverage import process_startup
    process_startup.coverage.save()
    orig(*args)


os.execlp = patched_execlp
