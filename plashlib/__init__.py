
import os
if os.environ.get('PLASH_KILL_PARENT_ON_EXCEPTION'):
    import sys, traceback, signal
    def my_except_hook(exctype, value, traceb):
        traceback.print_exception(exctype, value, traceb)
        os.kill(os.getppid(), signal.SIGKILL)
        sys.exit(1) # should not hit this line
    sys.excepthook = my_except_hook
