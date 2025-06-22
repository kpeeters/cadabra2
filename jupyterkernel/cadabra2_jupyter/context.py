from io import StringIO
import sys, os
import cadabra2

# super important
__cdbkernel__ = cadabra2.__cdbkernel__

#  setup stdout, stderr hook
class _StdCatch:
    def __init__(self, kernel):
        self._kernel = kernel

    def __enter__(self):
        sys.stdout = self.stdout = StringIO()
        sys.stderr = self.stderr = StringIO()

    def __exit__(self, exc_type, exc_val, exc_traceback):
        sys.stdout = sys.__stdout__
        sys.stderr = sys.__stderr__

        for line in self.stdout.getvalue().splitlines():
            # insert missing newline
            self._kernel._send_code(line + "\n")

        # ignore exc_type reporting, since it always gives 'JSON serializable'
        # error, echoing the same message as provided by the __stderr__ catch
        for line in self.stderr.getvalue().splitlines():
            # insert missing newline
            self._kernel._send_error(line + "\n")


class SandboxContext:
    def __init__(self, kernel):
        self._sandbox = {
            "server": kernel._cdb_server,
            "__cdbkernel__": cadabra2.__cdbkernel__,
        }
        # Since we have been able to `import cadabra2`, we know where
        # that module is located. The `cadabra2_defaults.py` is at the
        # same location.
        with open(os.path.join(os.path.dirname(cadabra2.__file__), "cadabra2_defaults.py")) as f:
            code = compile(f.read(), "cadabra2_defaults.py", "exec")
        exec(code, self._sandbox)

        self._kernel = kernel
        self._context = _StdCatch(kernel)

    def __call__(self, code):
        # redefine, as is catastrophic if accidentally overwritten
        self._sandbox["server"] = self._kernel._cdb_server
        with self._context:
            exec(code, self._sandbox)

    @property
    def namespace(self):
        return self._sandbox.keys()
