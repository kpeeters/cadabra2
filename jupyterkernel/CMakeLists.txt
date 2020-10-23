cmake_minimum_required(VERSION ${CADABRA_CMAKE_VERSION})
project(Cadabra)

print_header("Configuring Py Cadabra Jupyter Kernel")
set(
  PY_CADABRA_JUPYTER_VERSION "1.0.1"
)

if(PYTHON_POSTFIX MATCHES 3)
  message(STATUS "Python version 3.")
else()
  message(STATUS "Cannot build Py Cadabra Jupyter Kernel with Python version 2.")
  return()
endif()

message(STATUS "Py Cadabra Jupyter version ${PY_CADABRA_JUPYTER_VERSION}.")

configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/kernelspec/kernel.json.in"
	"${CMAKE_CURRENT_SOURCE_DIR}/kernelspec/kernel.json"
)
configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/cadabra2_jupyter/__init__.py.in"
	"${CMAKE_CURRENT_SOURCE_DIR}/cadabra2_jupyter/__init__.py"
)

install(
  CODE "execute_process(WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} COMMAND ${PYTHON_EXECUTABLE} -m pip install --user .)"
)
