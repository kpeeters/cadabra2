#
# Find python libraries
#
# On Apple the standard CMake PythonLibs.cmake does not
# work correctly, because it will always give some paths which point
# to the Apple-supplied Python libraries. We need the Homebrew
# installed ones. In order to achieve this, we use CMake scripts which
# were taken from Nikolaus Dremmel's repo at github.com/NikolausDemmel/CMake.
#

if(WIN32)
  set(STATIC_LIB_SUFFIX "lib")
  set(SHARED_LIB_SUFFIX "dll")
  set(PYTHON_MOD_SUFFIX "pyd")
else()
  set(STATIC_LIB_SUFFIX "a")
  set(SHARED_LIB_SUFFIX "so")
  set(PYTHON_MOD_SUFFIX "so")
endif()

if(USE_PYTHON_3)
  set(Python_ADDITIONAL_VERSIONS 3.4 3.5 3.6)
  set(PythonInterp_ADDITIONAL_VERSIONS 3.4 3.5 3.6)
  find_package(PythonInterp 3.4 REQUIRED)
  if(APPLE)
     find_package(PythonLibsOSX 3.4 REQUIRED)
  else()
     find_package(PythonLibsNew 3.4 REQUIRED)
  endif()
else()
  set(Python_ADDITIONAL_VERSIONS 2.7)
  set(PythonInterp_ADDITIONAL_VERSIONS 2.7)
  find_package(PythonInterp 2.7 REQUIRED)
  if(APPLE)
     find_package(PythonLibsOSX 2.7 REQUIRED)
  else()
     find_package(PythonLibsNew 2.7 REQUIRED)
  endif()
endif()
message("-- Found Python ${PYTHON_LIBRARIES}")

if("${PYTHON_SITE_PATH}" STREQUAL "")
  execute_process(COMMAND ${PYTHON_EXECUTABLE} -c "import site; print (site.getsitepackages()[0]);"
                  OUTPUT_VARIABLE PYTHON_SITE_PATH OUTPUT_STRIP_TRAILING_WHITESPACE)
endif()

message("-- Python site path at ${PYTHON_SITE_PATH}")
