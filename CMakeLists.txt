set(CMAKE_LEGACY_CYGWIN_WIN32 0)
cmake_minimum_required(VERSION 3.1)
set(CMAKE_CXX_STANDARD 14)
project(Cadabra)

#---------------------------------------------------------------------------
# Preamble
#---------------------------------------------------------------------------

# Debug messaging function
include(cmake/debug_message.cmake)

# Set path to additional cmake files
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules")

# Get version information. 
include(cmake/version.cmake)
message("-- Building ${CADABRA_VERSION_MAJOR}.${CADABRA_VERSION_MINOR}.${CADABRA_VERSION_PATCH}")
message("-- Build id ${CADABRA_VERSION_BUILD} dated ${CADABRA_VERSION_DATE}")

# Allows tests to be built in all subdirectories.
enable_testing()

# Provide sensible defaults on Windows
if (WIN32)
  if (NOT DEFINED CMAKE_BUILD_TYPE)
    if (DEFINED CMAKE_CONFIGURATION_TYPES)
	  set(CMAKE_BUILD_TYPE ${CMAKE_CONFIGURATION_TYPES})
	  message(" -- No build type specified, defaulting to ${CMAKE_CONFIGURATION_TYPES} specified in CMAKE_CONFIGURATION_TYPES")
	else()
      set(CMAKE_BUILD_TYPE RelWithDebInfo)
	  message(" -- No build type specified, defaulting to RelWithDebInfo")
	endif()
  endif()
  if (NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    message(" -- CMAKE_TOOLCAHIN_FILE not specified, attempting to find vcpkg.cmake")
    find_file(CMAKE_TOOLCHAIN_FILE vcpkg.cmake HINTS $ENV{userprofile} $ENV{systemdrive} PATH_SUFFIXES vcpkg/scripts/buildsystems)
  endif()
  if (NOT CMAKE_TOOLCHAIN_FILE)
    message(FATAL_ERROR " -- Could not find vcpkg. Currently building Cadabra on Windows requires vcpkg, please set the path to vcpkg.cmake using the -DCMAKE_TOOLCHAIN_FILE flag")
  endif()
endif()

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
message("-- CMAKE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}")

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
option (MINIMUM_INSTALLATION "Only copy compiled filed to install dir" OFF)
#---------------------------------------------------------------------------
# Compiler flags.
#---------------------------------------------------------------------------

# - Set the C++ standard to C++14
# - Turn optimizations on
# - Turn off warnings we don't need

include(cmake/winlibs.cmake)

debug_message("Available build types : ${CMAKE_CONFIGURATION_TYPES}")
debug_message("Initial compiler flags: ${CMAKE_CXX_FLAGS}")

# GCC
if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O2 -Wall -fvisibility=hidden -Wno-unused-but-set-variable")
endif()

# Clang
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  # For Clang, need to additionally check version to avoid compiler bugs
  if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 3.5)
    message(FATAL_ERROR "Clang version must be at least 3.5 to avoid known bugs.")
  endif()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O2 -fvisibility=hidden -Wall -Wextra -Wunused")
endif() 

# Visual Studio
if(MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++14 /O2")
  # Force to always compile with /w (disables warnings)
  if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
    string(REGEX REPLACE "/W[0-4]" "/w" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /w")
  endif()
endif()

debug_message("Final compiler flags: ${CMAKE_CXX_FLAGS}")

#---------------------------------------------------------------------------
# Configure the various parts of Cadabra.
#---------------------------------------------------------------------------

if(ENABLE_MATHEMATICA)
  find_package(Mathematica COMPONENTS WSTP)
endif()

# Make sure the build type is non-empty.
if(WIN32)
  set(CADABRA_BUILD_TYPE "${CMAKE_BUILD_TYPE}")
  if(${CADABRA_BUILD_TYPE}=="")
	 set(CADABRA_BUILD_TYPE "Debug")
  endif()
endif()

# Include packaging logic.
include(cmake/packaging.cmake)

# Generate config file with CMake variables.
configure_file("${PROJECT_SOURCE_DIR}/core/Config.hh.in" "${PROJECT_SOURCE_DIR}/core/Config.hh")

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
# We install the python module in our 'share' path, not in
# the site-wide path as we used to. 
set(PYTHON_SITE_PATH ${CMAKE_INSTALL_PREFIX}/share/cadabra2/python)
if(WIN32)
else()
  execute_process(COMMAND ${PYTHON_EXECUTABLE} -c "import site; print (site.getsitepackages()[0]);"
    OUTPUT_VARIABLE OLD_PYTHON_SITE_PATH OUTPUT_STRIP_TRAILING_WHITESPACE)
endif()
message("-- Python site path ${OLD_PYTHON_SITE_PATH}")
message("-- Python packages to be installed at ${PYTHON_SITE_PATH}")
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
