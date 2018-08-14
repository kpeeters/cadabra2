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
import os
os.environ.setdefault('PATH', '')

class PackageCompiler(object):
	def find_module(self, fullname, path=None):
		# Top-level import if path=None
		if path is None or path == "":
			path = [os.getcwd()]
		# Get unqualified package name
		if '.' in fullname:
			*parents, name = fullname.split('.')
		else:
			name = fullname
		# Go through path and try to find a notebook. If found, compile
		for entry in path:
			if os.path.isfile(os.path.join(entry, name + ".cnb")):
				compile_package__(os.path.join(entry, name))
				break;
			
		
		# Always return None. This passes on the onus of actually importing
		# the package to Python, which can do this much more efficiently
		return None

# Prepend to sys.meta_path, so that all imports will first be checked in
# case they are notebooks that need compiling
sys.meta_path.insert(0, PackageCompiler())

# Add current directory to Python module import path.
sys.path.append(".")

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
    class Server:
        """!@brief Object to handle advanced display in a UI-independent way.

        @long Cadabra makes available to Python a Server object, which
        contains functions to send output to the user. When running
        from the command line this simply prints to the screen, but it
        can talk to a remote client to display images and maths.
        """
        
        def send(self, data, typestr, parent_id, last_in_sequence):
            """ Send a message to the client; 'typestr' indicates the cell type,
            'parent_id', if non-null, indicates the serial number of the parent
            cell.
            """
            print(data)
            return 0

        def architecture(self):
            return "terminal"

        def test(self):
            print("hello there!")

        def handles(self, otype):
            if(otype=="plain"):
                return True
            return False

        def totals(self):
            return __cdb_progress_monitor__.totals()
            
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
    """
    Generalised 'print' function which knows how to display objects in the 
    best possible way on the used interface, be it a console or graphical
    notebook. In particular, it knows how to display Cadabra expressions
    in typeset form whenever LaTeX functionality is available. Can also be
    used to display matplotlib plots.

    When using a Cadabra front-end (command line or notebook), an expression
    with a trailing semi-colon ';' will automatically be wrapped in a 
    'display' function call so that the expression is displayed immediately.
    """
    if 'matplotlib' in sys.modules and isinstance(obj, matplotlib.figure.Figure):
        imgstring = io.BytesIO()
        obj.savefig(imgstring,format='png')
        imgstring.seek(0)
        b64 = base64.b64encode(imgstring.getvalue())
        server.send(b64, "image_png", 0, False)
        # FIXME: Use the 'handles' query method on the Server object
        # to figure out whether it can do something useful
        # with a particular data type.

    elif 'matplotlib' in sys.modules and isinstance(obj, matplotlib.artist.Artist):
        f = obj.get_figure()
        imgstring = io.BytesIO()
        f.savefig(imgstring,format='png')
        imgstring.seek(0)
        b64 = base64.b64encode(imgstring.getvalue())
        server.send(b64, "image_png", 0, False)

    elif hasattr(obj,'_backend'):
        if hasattr(obj._backend,'fig'):
            f = obj._backend.fig
            imgstring = io.BytesIO()
            f.savefig(imgstring,format='png')
            imgstring.seek(0)
            b64 = base64.b64encode(imgstring.getvalue())
            server.send(b64, "image_png", 0, False)

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
                id=server.send(ret, "latex_view", 0, False)
                # print(id)
                # Make a child cell of the above with input form content.
                server.send(obj.input_form(), "input_form", id, False)                
        else:
            server.send(str(obj), "plain", 0, False)

    elif isinstance(obj, Property):
        if server.handles('latex_view'):
            ret = mopen+obj._latex_()+mclose
            if delay_send:
                return ret
            else:
                server.send(ret , "latex_view", 0, False)
                # Not yet available.
                # server.send(obj.input_form(), "input_form", 0, False)
        else:
            server.send(str(obj), "plain", 0, False)
            
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
        server.send(out, "latex_view", 0, False)
        # FIXME: send input_form version.
        
    elif hasattr(obj, "__module__") and hasattr(obj.__module__, "find") and obj.__module__.find("sympy")!=-1:
        server.send("\\begin{dmath*}{}"+latex(obj)+"\\end{dmath*}", "latex_view", 0, False)
        
    else:
        # Failing all else, just dump a str representation to the notebook, asking
        # it to display this verbatim.
        # server.send("\\begin{dmath*}{}"+str(obj)+"\\end{dmath*}", "latex")
        if delay_send:
            return "\\verb|"+str(obj)+"|"
        else:
            server.send(str(obj), "verbatim", 0, False)
    
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

