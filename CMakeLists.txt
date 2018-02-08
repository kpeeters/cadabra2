set(CMAKE_LEGACY_CYGWIN_WIN32 0)
cmake_minimum_required(VERSION 3.1)
set(CMAKE_CXX_STANDARD 14)
project(Cadabra)

#---------------------------------------------------------------------------
# Preamble
#---------------------------------------------------------------------------

# Set path to additional cmake files
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules")

# Get version information. 
include(cmake/version.cmake)
message("-- Building ${CADABRA_VERSION_MAJOR}.${CADABRA_VERSION_MINOR}.${CADABRA_VERSION_PATCH}")
message("-- Build id ${CADABRA_VERSION_BUILD} dated ${CADABRA_VERSION_DATE}")

# Allows tests to be built in all subdirectories.
enable_testing()

#---------------------------------------------------------------------------
# User options.
#---------------------------------------------------------------------------

# Provide option to build with Python 3 (default) or Python 2.
option(USE_PYTHON_3 "Use Python 3 if ON, or fall back to Python 2 if OFF" ON)
if(USE_PYTHON_3)
  set(PYTHON_POSTFIX "3")
else()
  set(PYTHON_POSTFIX "")
endif()

option(PACKAGING_MODE "Run in packaging mode, overriding path settings" OFF)
if(PACKAGING_MODE)
  message("-- Packaging mode")
  if("${CMAKE_INSTALL_PREFIX}" STREQUAL "/usr")
  else()
	 MESSAGE(FATAL_ERROR "Building with -DPACKAGING_MODE=ON also requires -DCMAKE_INSTALL_PREFIX=/usr")
  endif()
else()
  message("-- User mode")
endif()

# Ensure that all installed binaries have an RPATH setting
# which enables them to find libtexengine, libcadabra_client and
# libcadabra_server without any ld.so.conf settings.

SET(CMAKE_SKIP_BUILD_RPATH  FALSE)
SET(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE) 
SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/config/postinst.in" "${CMAKE_CURRENT_BINARY_DIR}/postinst" @ONLY)

if(USE_PYTHON_3)
  message("-- Building for use with Python 3 (good!)")
else()
  message("-- Building for use with Python 2 (consider upgrading!)")
endif()

option(ENABLE_MATHEMATICA "Enable Mathematica support" ON)
option(ENABLE_FRONTEND    "Enable the UI frontend" ON)

#---------------------------------------------------------------------------
# Configure the various parts of Cadabra.
#---------------------------------------------------------------------------

if(ENABLE_MATHEMATICA)
  find_package(Mathematica COMPONENTS WSTP)
endif()

# Include packaging logic.
include(cmake/packaging.cmake)

# Generate config file with CMake variables.
configure_file("${PROJECT_SOURCE_DIR}/core/Config.hh.in" "${PROJECT_BINARY_DIR}/core/Config.hh")
include_directories(${PROJECT_BINARY_DIR}/core)

# Add all sub-projects to build process.
add_subdirectory(libs/pybind11)
if(WIN32)
  set(STATIC_LIB_SUFFIX "lib")
  set(SHARED_LIB_SUFFIX "dll")
  set(PYTHON_MOD_SUFFIX "pyd")
else()
  set(STATIC_LIB_SUFFIX "a")
  set(SHARED_LIB_SUFFIX "so")
  set(PYTHON_MOD_SUFFIX "so")
endif()
if("${PYTHON_SITE_PATH}" STREQUAL "")
  execute_process(COMMAND ${PYTHON_EXECUTABLE} -c "import site; print (site.getsitepackages()[0]);"
                  OUTPUT_VARIABLE PYTHON_SITE_PATH OUTPUT_STRIP_TRAILING_WHITESPACE)
endif()
message("-- Python site path at ${PYTHON_SITE_PATH}")
message("-- Python module suffix \".${PYTHON_MOD_SUFFIX}\"")

add_subdirectory(core)
add_subdirectory(client_server)
if(ENABLE_FRONTEND)
  add_subdirectory(frontend)
endif()
add_subdirectory(tests)
add_subdirectory(web2 EXCLUDE_FROM_ALL)

# Provide uninstall target.
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/cmake/cmake_uninstall.cmake"
  IMMEDIATE @ONLY)
add_custom_target(uninstall
  "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake/cmake_uninstall.cmake") 
