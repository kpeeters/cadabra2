cmake_minimum_required(VERSION ${CADABRA_CMAKE_VERSION})
project(Cadabra)


# In a PyPI wheel, the kernel has to start with whichever Python the
# wheel got installed into; jupyter_client replaces a bare "python"
# by the interpreter it runs under.
if(PYPI_WHEEL)
	set(KERNEL_PYTHON_EXECUTABLE "python")
	set(KERNELSPEC_DIR "${SKBUILD_DATA_DIR}/share/jupyter/kernels/cadabra2")
else()
	set(KERNEL_PYTHON_EXECUTABLE "${Python_EXECUTABLE}")
	set(KERNELSPEC_DIR "share/jupyter/kernels/cadabra2")
endif()

configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/kernelspec/kernel.json.in"
	"${CMAKE_CURRENT_SOURCE_DIR}/kernelspec/kernel.json"
)
configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/cadabra2_jupyter/__init__.py.in"
	"${CMAKE_CURRENT_SOURCE_DIR}/cadabra2_jupyter/__init__.py"
)

install(FILES
   kernelspec/kernel.json
   kernelspec/logo-32x32.png
   kernelspec/logo-64x64.png
   DESTINATION
   "${KERNELSPEC_DIR}"
   )

install(FILES
   cadabra2_jupyter/__init__.py
   cadabra2_jupyter/__main__.py
   cadabra2_jupyter/context.py
   cadabra2_jupyter/completer.py
   cadabra2_jupyter/extension.py
   cadabra2_jupyter/kernel.py
   cadabra2_jupyter/server.py
   DESTINATION
   "${PYTHON_SITE_PATH}/cadabra2_jupyter"
   )

# The CodeMirror lexer goes into the classic notebook's directory tree,
# which a wheel should not touch.
if(NOT PYPI_WHEEL)
  install(FILES
    lexer/cadabra.js
    DESTINATION
    "${PYTHON_SITE_PATH}/notebook/static/components/codemirror/mode/cadabra"
    )
endif()
