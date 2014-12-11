# This is a pure-python initialisation script to set the 
# path to sympy and setup printing of cadabra expressions.

import sys
from cadabra2 import *
sys.path.insert(0,'/home/kasper/Development/git.others/sympy') 

# Attempt to import sympy; if not, setup logic so that the
# shell does not fail later.

try:
    import sympy
except:
    class Sympy:
        __version__="unavailable"

    sympy = Sympy()


# Set display hooks to catch certain objects and print them
# differently. Should probably eventually be done cleaner.

def _displayhook(arg):
    global remember_display_hook
    if isinstance(arg, Ex):
        print(str(arg))
    elif isinstance(arg, Property):
        print(str(arg))
    else:
        remember_display_hook(arg)

remember_display_hook = sys.displayhook
sys.displayhook = _displayhook

