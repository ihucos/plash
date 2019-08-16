import os 
import ctypes

dir_path = os.path.dirname(os.path.realpath(__file__))
clib = os.path.realpath(os.path.join(dir_path, '../../c/plash.o'))

lib = ctypes.CDLL(clib)

def setup_user_ns()
    lib.pl_setup_user_ns()
