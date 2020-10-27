cmake_minimum_required(VERSION ${CADABRA_CMAKE_VERSION})
project(Cadabra)


if(PYTHON_POSTFIX MATCHES 2)
   message(FATAL "Cannot build pure-Python Cadabra Jupyter kernel with Python version 2.")
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
   share/jupyter/kernels/cadabra2
   )

install(FILES
   cadabra2_jupyter/__init__.py
   cadabra2_jupyter/__main__.py
   cadabra2_jupyter/context.py
   cadabra2_jupyter/completer.py
   cadabra2_jupyter/kernel.py
   cadabra2_jupyter/server.py
   DESTINATION
   "${PYTHON_SITE_PATH}/cadabra2_jupyter"
   )

install(FILES
  lexer/cadabra.js
  DESTINATION
  "${PYTHON_SITE_PATH}/notebook/static/components/codemirror/mode/cadabra"
  )
