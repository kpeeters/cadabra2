from io import StringIO
import sys, os
import cadabra2

from cadabra2_jupyter import SITE_PATH

# super important
__cdbkernel__ = cadabra2.__cdbkernel__

server = None  #  require so that server is in global namespace
#  import cadabra2 defaults programatically so it shares global namespace
with open(os.path.join(SITE_PATH, "cadabra2_defaults.py")) as f:
    code = compile(f.read(), "cadabra2_defaults.py", "exec")
exec(code, globals())

def _attatch_kernel_server(instance):
    global server
    server = instance

def _exec_in_context(code):
    exec(code, globals())
