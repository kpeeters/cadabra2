# Generate the type stub 'cadabra2.pyi' for the compiled cadabra2
# module with pybind11-stubgen, so that static checkers (Pyright in
# Google Colab and VS Code, mypy, ...) know which names the module
# provides and what their signatures are.
#
# Run as a script, from the custom command in core/CMakeLists.txt:
#
#    cmake -DPython_EXECUTABLE=... -DMODULE_DIR=... -DPACKAGES_DIR=...
#          -DAPPDIRS_DIR=... -DMANUAL_DIR=... -DOUTPUT_DIR=...
#          -P stubgen.cmake
#
# MODULE_DIR is where the built cadabra2 module lives, PACKAGES_DIR the
# build folder with 'cadabra2_defaults.py' and 'cdb/main.py' (which the
# module needs at import time), MANUAL_DIR the folder with the
# 'algorithms' and 'properties' manual pages from which the docstrings
# are taken.
#
# A failure only produces a warning: the stub is useful, but the module
# works without it.

if(WIN32)
  set(sep ";")
else()
  set(sep ":")
endif()
set(ENV{PYTHONPATH} "${MODULE_DIR}${sep}${PACKAGES_DIR}${sep}${APPDIRS_DIR}")
set(ENV{CADABRA_MANUAL_DIR} "${MANUAL_DIR}")

file(REMOVE "${OUTPUT_DIR}/cadabra2.pyi")
execute_process(
  COMMAND "${Python_EXECUTABLE}" -m pybind11_stubgen cadabra2
          --ignore-all-errors
          --output-dir "${OUTPUT_DIR}"
  RESULT_VARIABLE result
)
if(NOT result EQUAL 0 OR NOT EXISTS "${OUTPUT_DIR}/cadabra2.pyi")
  file(REMOVE "${OUTPUT_DIR}/cadabra2.pyi")
  message(WARNING "pybind11-stubgen failed (${result}); not generating cadabra2.pyi.")
endif()
