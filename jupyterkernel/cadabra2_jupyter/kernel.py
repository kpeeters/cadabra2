import ipykernel.kernelbase
import sys
import traceback

import cadabra2
from cadabra2_jupyter.context import _exec_in_context, _attatch_kernel_server
from cadabra2_jupyter.server import Server
from cadabra2_jupyter import __version__


class CadabraJupyterKernel(ipykernel.kernelbase.Kernel):
    implementation = "cadabra_kernel"
    implementation_version = __version__
    language_info = {
        "name": "cadabra2",
        "codemirror_mode": "python",
        "pygments_lexer": "python",
        "mimetype": "text/python",
        "file_extension": ".ipynb",
    }

    @property
    def banner(self):
        return "Info at http://cadabra.science/\nAvailable under the terms of the GNU General Public License v3"

    def __init__(self, **kwargs):
        ipykernel.kernelbase.Kernel.__init__(self, **kwargs)
        self._parse_cadabra = cadabra2.cdb2python_string

        # attach the server class for callbacks
        self._cdb_server = Server(self)
        _attatch_kernel_server(self._cdb_server)

    def do_execute(
        self, code, silent, store_history=True, user_expressions=None, allow_stdin=False
    ):
        self.silent = silent
        # check for blank input
        if not code.strip():
            return {
                "status": "ok",
                "execution_count": self.execution_count,
                "payload": [],
                "user_expressions": {},
            }
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
            return {
                "status": "ok",
                "execution_count": self.execution_count,
                "payload": [],
                "user_expressions": {},
            }

    def _execute_python(self, pycode):
        """ executes python code in the cadabra context """
        _exec_in_context(pycode)

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
