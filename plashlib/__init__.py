'''
from plashlib.utils import catch_and_die
import sys

def my_except_hook(exctype, value, traceback):
     if exctype == OSError:
        with catch_and_die([OSError]):
            raise value
     else:
         sys.__excepthook__(exctype, value, traceback)
sys.excepthook = my_except_hook
'''
