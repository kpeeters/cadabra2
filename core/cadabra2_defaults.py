##
# \file    cadabra2_defaults.py
# \ingroup pythoncore
# Cadabra2 pure Python functionality.
#
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
        """!@brief Stub object for when Sympy itself is not available.
        
        @long When Sympy is not available, this object contains some basic
        functionality to prevent things from breaking elsewhere.
        """
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

# Whether running in command-line mode or as client-server, there always
# needs to be a Server object known as 'server' through which interaction
# with the display routines is handled. The 'display' function will
# call the 'server.send' method.

if 'server' in globals():
    mopen="\\begin{dmath*}{}";
    mclose="\\end{dmath*}";
else:
    mopen=''
    mclose=''
    class Server(ProgressMonitor):
        """!@brief Object to handle advanced display in a UI-independent way.

        @long Cadabra makes available to Python a Server object, which
        contains functions to send output to the user. When running
        from the command line this simply prints to the screen, but it
        can talk to a remote client to display images and maths.
        """
        
        def send(self, data, typestr):
            print(data)

        def architecture(self):
            return "terminal"

        def test(self):
            print("hello there!")

        def handles(self, otype):
            if(otype=="plain"):
                return True
            return False            
            
    server = Server()

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

## @brief Generic display function which handles local as well as remote clients.
#
# The 'display' function is a replacement for 'str', in the sense that
# it will generate human-readable output. However, in contrast to
# 'str', it knows about what the front-end ('server') can display, and
# will adapt the output to that. For instance, if
# server.handles('latex_view') is true, it will generate LaTeX output,
# while it will generate just plain text otherwise.
# 
# Once it has figured out which display is accepted by 'server', it
# will call server.send() with data depending on the object type it is
# being fed. Data types the server object can support are:
# 
# - "latex_view": text-mode LaTeX string.
# - "image_png":  base64 encoded png image.
# - "verbatim":   ascii string to be displayed verbatim.

def display(obj, delay_send=False):
    if 'matplotlib' in sys.modules and isinstance(obj, matplotlib.figure.Figure):
        imgstring = io.BytesIO()
        obj.savefig(imgstring,format='png')
        imgstring.seek(0)
        b64 = base64.b64encode(imgstring.getvalue())
        server.send(b64, "image_png")
        # FIXME: Use the 'handles' query method on the Server object
        # to figure out whether it can do something useful
        # with a particular data type.

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
        if server.handles('latex_view'):
            ret = mopen+obj._latex_()+mclose
            if delay_send:
                return ret
            else:
                server.send(ret, "latex_view")
        else:
            server.send(str(obj), "plain")

    elif isinstance(obj, Property):
        if server.handles('latex_view'):
            ret = mopen+obj._latex_()+mclose
            if delay_send:
                return ret
            else:
                server.send(ret , "latex_view")
        else:
            server.send(str(obj), "plain")
            
    elif type(obj)==list:
        out="{}$\\big[$"
        first=True
        for elm in obj:
            if first==False:
                out+=",\discretionary{}{}{} "
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
    
__cdbkernel__.server=server
__cdbkernel__.display=display
    
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

# Default post-processing algorithms. These are not pre-processed
# so need to have the '__cdbkernel__' argument.

def post_process(__cdbkernel__, ex):
    collect_terms(ex)

