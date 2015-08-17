# This is a pure-python initialisation script to set the  path to
# sympy and setup printing of Cadabra expressions.  This script is
# called both by the command line interface 'cadabra2' as well as by
# the GUI backend server 'cadabra-server'.

import sys
from cadabra2 import *
#sys.path.insert(0,'/home/kasper/Development/git.others/sympy') 

# Attempt to import sympy; if not, setup logic so that the
# shell does not fail later.

try:
    import sympy
except:
    class Sympy:
        __version__="unavailable"

    sympy = Sympy()

if sympy.__version__ != "unavailable":
    from sympy import factor
    from sympy import integrate
    from sympy import diff
    from sympy import expand
    from sympy import symbols
    from sympy import latex
    from sympy import sin, cos, tan, trigsimp
    from sympy import Matrix

# Import matplotlib and setup functions to prepare its output
# for sending as base64 to the client. Example use:
#
#   import matplotlib.pyplot as plt
#   p = plt.plot([1,2,3],[1,2,5],'-o')
#   display(p[0])
#
import matplotlib
import matplotlib.artist
import StringIO
import base64
matplotlib.use('Agg')

# FIXME: it is not a good idea to have this pollute the global namespace.
#
# Generate a JSON object for sending to the client. This function
# does different things depending on the object type it is being
# fed.

def display(obj):
    if isinstance(obj, matplotlib.artist.Artist):
        f = obj.get_figure()
        imgstring = StringIO.StringIO()
        f.savefig(imgstring,format='png')
        imgstring.seek(0)
        b64 = base64.b64encode(imgstring.getvalue())
        server.send(b64,"image_png")

    if isinstance(obj, Ex):
        send_message('hello Ex:'+str(Ex))

    
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

