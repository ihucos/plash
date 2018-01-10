

#
# magically hide tracebacks when the user Ctrl-c
#
import signal
import os
import sys

def interrupt(a, b):
    sys.exit(130)
signal.signal(signal.SIGINT, interrupt)
