# Smoke test for a freshly built cadabra2 wheel (CIBW_TEST_COMMAND).
# Checks the Python module, its manual pages, the Jupyter kernelspec
# and the IPython extension, without needing a running notebook server.

import cadabra2

ex = cadabra2.Ex(r"A_{m n} + B_{m n}")
print("Module OK:", ex)

# The docstrings are read from the manual pages in the wheel's data directory.
doc = cadabra2.distribute.__doc__ or ""
assert "Failed to collect help" not in doc, doc
print("Manual pages OK")

from jupyter_client.kernelspec import KernelSpecManager

specs = KernelSpecManager().find_kernel_specs()
assert "cadabra2" in specs, "cadabra2 kernelspec not found, have {}".format(list(specs))
print("Kernelspec OK:", specs["cadabra2"])

from IPython.core.interactiveshell import InteractiveShell

shell = InteractiveShell.instance()
shell.run_line_magic("load_ext", "cadabra2_jupyter")
# Loading twice must be harmless.
shell.run_line_magic("load_ext", "cadabra2_jupyter")

res = shell.run_cell("ex:=(A+B) (C+D);\ndistribute(ex);")
assert res.success, "Cadabra cell failed: {}".format(res.error_in_exec or res.error_before_exec)
assert str(shell.user_ns["ex"]) == str(cadabra2.Ex(r"A C + A D + B C + B D")), \
    "Unexpected result: {}".format(shell.user_ns["ex"])

# IPython magics and plain Python must still work with the extension loaded.
res = shell.run_cell("%time x = 1 + 1\ny = x * 2")
assert res.success and shell.user_ns["y"] == 4, "Plain Python cell failed"

print("IPython extension OK")
