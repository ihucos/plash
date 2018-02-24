#
# magically hide tracebacks when the user Ctrl-c
#
import sys


def interrupt(a, b):
    sys.exit(130)


#signal.signal(signal.SIGINT, interrupt)

import coverage; coverage.process_startup()
