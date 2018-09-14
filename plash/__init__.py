import os
from plash import utils

testmode_file = os.path.join(utils.get_plash_data(), 'config', 'testmode')
if os.path.exists(testmode_file):
    import sys, traceback, signal

    def my_except_hook(exctype, value, traceb):
        traceback.print_exception(exctype, value, traceb)
        os.kill(os.getppid(), signal.SIGKILL)
        sys.exit(1)  # should not hit this line

    sys.excepthook = my_except_hook
