# This is a pure-python initialisation script to set the  path to
# sympy and setup printing of Cadabra expressions.  This script is
# called both by the command line interface 'cadabra2' as well as by
# the GUI backend server 'cadabra-server'.

import sys
import cadabra2
from cadabra2 import *
__cdbkernel__=cadabra2.__cdbkernel__

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
    from sympy import symbols
    from sympy import latex
    from sympy import sin, cos, tan, sqrt, trigsimp
    from sympy import Matrix as sMatrix

# Import matplotlib and setup functions to prepare its output
# for sending as base64 to the client. Example use:
#
#   import matplotlib.pyplot as plt
#   p = plt.plot([1,2,3],[1,2,5],'-o')
#   display(p[0])
#

have_matplotlib=True
try:
    import matplotlib
    import matplotlib.artist
    import matplotlib.figure
    matplotlib.use('Agg')
except ImportError:
    have_matplotlib=False

import io
import base64

# Generate a JSON object for sending to the client. This function
# does different things depending on the object type it is being
# fed. Data types the server object understands:
#
#  - "latex_view": text-mode LaTeX string.
#  - "image_png":  base64 encoded png image.
#  - "verbatim":   ascii string to be displayed verbatim.

mopen="\\begin{dmath*}{}";
mclose="\\end{dmath*}";

def display(obj, delay_send=False):
    if 'matplotlib' in sys.modules and isinstance(obj, matplotlib.figure.Figure):
        imgstring = io.BytesIO()
        obj.savefig(imgstring,format='png')
        imgstring.seek(0)
        b64 = base64.b64encode(imgstring.getvalue())
        server.send(b64, "image_png")

    elif 'matplotlib' in sys.modules and isinstance(obj, matplotlib.artist.Artist):
        f = obj.get_figure()
        imgstring = io.BytesIO()
        f.savefig(imgstring,format='png')
        imgstring.seek(0)
        b64 = base64.b64encode(imgstring.getvalue())
        server.send(b64, "image_png")

    elif hasattr(obj,'_backend'):
        if hasattr(obj._backend,'fig'):
            f = obj._backend.fig
            imgstring = io.BytesIO()
            f.savefig(imgstring,format='png')
            imgstring.seek(0)
            b64 = base64.b64encode(imgstring.getvalue())
            server.send(b64, "image_png")

    elif 'vtk' in sys.modules and isinstance(obj, vtk.vtkRenderer):
        # Vtk renderer, see http://nbviewer.ipython.org/urls/bitbucket.org/somada141/pyscience/raw/master/20140917_RayTracing/Material/PythonRayTracingEarthSun.ipynb
        pass
                    
#    elif isinstance(obj, numpy.ndarray):
#        server.send("\\begin{dmath*}{}"+str(obj.to_list())+"\\end{dmath*}", "latex")

    elif isinstance(obj, Ex):
        if 'server' in globals():
            ret = mopen+obj._latex_()+mclose
            if delay_send:
                return ret
            else:
                server.send(ret, "latex_view")
        else:
            print(obj.__str__());

    elif isinstance(obj, Property):
        if 'server' in globals():
            ret = mopen+obj._latex_()+mclose
            if delay_send:
                return ret
            else:
                server.send(ret , "latex_view")
        else:
            print(obj.__str__())

    elif type(obj)==list:
        out="{}$\\big[$"
        first=True
        for elm in obj:
            if first==False:
                out+=", "
            else:
                first=False
            out+= display(elm, True)
        out+="$\\big]$";
        server.send(out, "latex_view")
        
    elif hasattr(obj, "__module__") and hasattr(obj.__module__, "find") and obj.__module__.find("sympy")!=-1:
        server.send("\\begin{dmath*}{}"+latex(obj)+"\\end{dmath*}", "latex_view")
        
    else:
        # Failing all else, just dump a str representation to the notebook, asking
        # it to display this verbatim.
        # server.send("\\begin{dmath*}{}"+str(obj)+"\\end{dmath*}", "latex")
        if delay_send:
            return "\\verb|"+str(obj)+"|"
        else:
            server.send(str(obj), "verbatim")
    
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

# Default post-processing algorithms.

def post_process(ex):
    collect_terms(ex)

