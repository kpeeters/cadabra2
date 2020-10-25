import ipykernel.kernelbase
import sys
import traceback
import re
import string

import cadabra2
from cadabra2_jupyter.context import SandboxContext
from cadabra2_jupyter.server import Server
from cadabra2_jupyter import __version__

_accepted_chars = string.ascii_letters


class CadabraJupyterKernel(ipykernel.kernelbase.Kernel):
    implementation = "cadabra_kernel"
    implementation_version = __version__
    language_info = {
        "name": "cadabra2",
        "codemirror_mode": "cadabra2",
        "pygments_lexer": "cadabra2",
        "mimetype": "text/cadabra2",
        "file_extension": ".ipynb",
    }

    @property
    def banner(self):
        return "Cadabra (C) 2001-2020 Kasper Peeters\nJupyter kernel by Fergus Baker\nMore info at https://cadabra.science/\nAvailable under the terms of the GNU General Public License v3"

    def __init__(self, **kwargs):
        ipykernel.kernelbase.Kernel.__init__(self, **kwargs)
        self._parse_cadabra = cadabra2.cdb2python_string

        # attach the server class for callbacks
        self._cdb_server = Server(self)

        # init the sandbox
        self._sandbox_context = SandboxContext(self)

    def do_execute(
        self, code, silent, store_history=True, user_expressions=None, allow_stdin=False
    ):
        """ callback for iPython kernel: code execution """
        self.silent = silent
        # check for blank input
        if not code.strip():
            return self._status_ok

        interrupted = False

        try:
            #  main execution calls
            pycode = self._parse_cadabra(code, True)
            self._execute_python(pycode)

        except KeyboardInterrupt:
            interrupted = True

        except Exception as e:
            # get traceback; not massively informative but can be useful
            err_str = traceback.format_exc()
            self._send_error(err_str)

        if interrupted:
            return {"status": "abort", "execution_count": self.execution_count}
        else:
            return self._status_ok

    def do_complete(self, code, cursor_pos):
        """ callback for iPython kernel: code completion """

        # if no code, or last character is not alphabetical
        if not code or code[cursor_pos - 1] not in _accepted_chars:
            return self._default_complete(cursor_pos)

        # only choose up until current cursor position
        code = code[:cursor_pos]

        # get last 'word' item
        last_item = re.compile(r"[\W\s]").split(code)[-1]

        # sandbox namespace
        namespace = self._sandbox_context.namespace

        # find valid options
        options = list(
            filter(lambda i: re.match("^{}.*".format(last_item), i), namespace)
        )
        return {
            "matches": sorted(options),
            "cursor_start": cursor_pos - len(last_item),
            "cursor_end": cursor_pos,
            "status": "ok",
            "metadata": dict(),
        }

    @property
    def _status_ok(self):
        return {
            "status": "ok",
            "execution_count": self.execution_count,
            "payload": [],
            "user_expressions": {},
        }

    def _default_complete(self, cursor_pos):
        return {
            "matches": [],
            "cursor_start": 0,
            "cursor_end": cursor_pos,
            "metadata": dict(),
            "status": "ok",
        }

    def _execute_python(self, pycode):
        """ executes python code in the cadabra context """
        self._sandbox_context(pycode)

    def _send_result(self, res_str):
        self.send_response(
            self.iopub_socket,
            "display_data",
            {"data": {"text/markdown": "{}".format(res_str)}, "metadata": {}},
        )

    def _send_image(self, img):
        raise NotImplementedError

    def _send_code(self, res_str):
        self.send_response(
            self.iopub_socket,
            "stream",
            {"name": "stdout", "text": res_str},
        )

    def _send_error(self, err_str):
        self.send_response(
            self.iopub_socket, "stream", {"name": "stderr", "text": err_str}
        )
