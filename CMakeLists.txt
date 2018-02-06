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

find_package(Mathematica COMPONENTS WSTP)

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

# alread taken care of by 
#include(cmake/locate_python.cmake)

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

option(ENABLE_FRONTEND "Enable the UI frontend" ON)

#---------------------------------------------------------------------------
# Configure the various parts of Cadabra.
#---------------------------------------------------------------------------

# Include packaging logic.
include(cmake/packaging.cmake)

# Generate config file with CMake variables.
configure_file("${PROJECT_SOURCE_DIR}/core/Config.hh.in" "${PROJECT_BINARY_DIR}/core/Config.hh")
include_directories(${PROJECT_BINARY_DIR}/core)

# Add all sub-projects to build process.
add_subdirectory(libs/pybind11)
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
