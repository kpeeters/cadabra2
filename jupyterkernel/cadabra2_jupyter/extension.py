# IPython extension which makes Cadabra input and output work in a
# standard IPython/Jupyter Python kernel, for environments where the
# Cadabra kernel cannot be installed (Google Colab, Kaggle, VS Code
# notebooks, ...). Enable it with
#
#    %load_ext cadabra2_jupyter
#
# after which cells can contain Cadabra notation (e.g. 'ex:=A+B;').
# The input is converted to Python with the same converter the Cadabra
# kernel uses, and output is sent through IPython's display machinery.

import base64
import os

import cadabra2
from IPython.display import display as _ipython_display

from cadabra2_jupyter.server import _latex_post_parser


class IPythonServer:
    """Stand-in for the Cadabra 'server' object, which the 'display'
    function in cadabra2_defaults.py uses to send output to the front-end.
    Output is passed on to IPython instead of to kernel sockets."""

    def send(self, data, typestr, parent_id, cell_id, last_in_sequence):
        if typestr == "latex_view":
            data = _latex_post_parser(data)
            _ipython_display({"text/markdown": data, "text/plain": data}, raw=True)
        elif typestr == "image_svg":
            _ipython_display({"image/svg+xml": base64.b64decode(data).decode("utf-8")}, raw=True)
        elif typestr == "image_png":
            if isinstance(data, bytes):
                data = data.decode("utf-8")
            _ipython_display({"image/png": data}, raw=True)
        elif typestr == "verbatim" or typestr == "plain":
            print(data)
        elif typestr == "input_form":
            pass
        else:
            raise Exception("Unknown typestr '{}'".format(typestr))
        # Return nothing, so that IPython does not show an 'Out[]'
        # value when 'display' is the last call in a cell.
        return None

    def architecture(self):
        return "ipython-extension"

    def test(self):
        self.send("Test: We've gone on holiday by mistake!", "verbatim", 0, 0, False)

    def handles(self, otype):
        return otype in ("latex_view", "image_png", "image_svg", "verbatim")

    def totals(self):
        return -1


def _cadabra_transformer(lines):
    """IPython input transformer: convert a cell from Cadabra to Python."""
    code = "".join(lines)
    if not code.strip():
        return lines
    pycode = cadabra2.cdb2python_string(code, True, "")
    return pycode.splitlines(keepends=True)


_loaded = False


def load(ip):
    global _loaded
    if _loaded:
        return

    ns = ip.user_ns
    ns["server"] = IPythonServer()
    ns["__cdbkernel__"] = cadabra2.__cdbkernel__

    # Since we have been able to `import cadabra2`, we know where that
    # module is located; `cadabra2_defaults.py` sits next to it. It picks
    # up the 'server' object set above.
    defaults = os.path.join(os.path.dirname(cadabra2.__file__), "cadabra2_defaults.py")
    with open(defaults) as f:
        code = compile(f.read(), defaults, "exec")
    exec(code, ns)

    # The 'post' transformers run after IPython has turned '%magic' and
    # '!shell' lines into plain Python, so the Cadabra converter only
    # ever sees Python and Cadabra notation.
    ip.input_transformers_post.append(_cadabra_transformer)
    _loaded = True


def unload(ip):
    global _loaded
    if _cadabra_transformer in ip.input_transformers_post:
        ip.input_transformers_post.remove(_cadabra_transformer)
    _loaded = False
