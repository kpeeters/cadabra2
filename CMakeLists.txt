set(CMAKE_LEGACY_CYGWIN_WIN32 0)
set(CADABRA_CMAKE_VERSION 3.9)

# Policy settings for CMake to resolve ambiguities.

if(POLICY CMP0042)
  cmake_policy(SET CMP0042 NEW)
endif()
if(POLICY CMP0054)
  cmake_policy(SET CMP0054 NEW)
endif()
if(POLICY CMP0127)
  cmake_policy(SET CMP0127 NEW)
endif()

cmake_minimum_required(VERSION ${CADABRA_CMAKE_VERSION})
if (${CMAKE_VERSION} VERSION_LESS "3.8.0")
	set(CMAKE_CXX_STANDARD 14)
	message(STATUS "Using C++14 as you are using CMake < 3.8; consider upgrading as future versions of Cadabra may use C++17 features")
else()
	set(CMAKE_CXX_STANDARD 17)
endif()
project(Cadabra)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(MACOS TRUE)
endif()

#---------------------------------------------------------------------------
# Preamble
#---------------------------------------------------------------------------

# Aliases for directories
set(CADABRA_ROOT_DIR ${CMAKE_SOURCE_DIR})
set(CADABRA_CLIENT_SERVER_DIR ${CADABRA_ROOT_DIR}/client_server)
set(CADABRA_CORE_DIR ${CADABRA_ROOT_DIR}/core)
set(CADABRA_FRONTEND_DIR ${CADABRA_ROOT_DIR}/frontend)
set(CADABRA_IMAGES_DIR ${CADABRA_ROOT_DIR}/images)
set(CADABRA_LIBS_DIR ${CADABRA_ROOT_DIR}/libs)

include(cmake/functions.cmake)

# Include Visual Studio specific build commands
if (MSVC)
  # https://developercommunity.visualstudio.com/content/problem/618088/cmake-msvc-toolset-version-is-incorrect-in-visual.html
  if ((MSVC_VERSION EQUAL 1921 OR MSVC_VERSION EQUAL 1922) AND MSVC_TOOLSET_VERSION EQUAL 141)
	 set(MSVC_TOOLSET_VERSION 142)
  endif()
  message(STATUS "MSVC_VERSION = ${MSVC_VERSION}, MSVC_TOOLSET_VERSION = ${MSVC_TOOLSET_VERSION}")

  include(cmake/windows.cmake)
endif()

# Make sure the build type is non-empty.
if (NOT DEFINED CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Debug")
endif()
set(CADABRA_BUILD_TYPE "${CMAKE_BUILD_TYPE}")
if (CMAKE_BUILD_TYPE MATCHES "^Debug$")
	set(CADABRA_DEBUG_BUILD TRUE)
endif()

# Set path to additional cmake files
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules")
if (APPLE)
   set(ENV{PKG_CONFIG_PATH} "/usr/local/opt/libffi/lib/pkgconfig:")
endif()

# Get version information.
include(cmake/version.cmake)
print_header("Building Cadabra version ${CADABRA_VERSION_MAJOR}.${CADABRA_VERSION_MINOR}.${CADABRA_VERSION_PATCH}.${CADABRA_VERSION_TWEAK} (${SYSTEM_BITS}-bit)")
message(STATUS "Build id '${CADABRA_VERSION_BUILD}' dated ${CADABRA_VERSION_DATE}")
message(STATUS "Build mode is set to '${CMAKE_BUILD_TYPE}'")

# Notify about install directory
if ("${CMAKE_INSTALL_PREFIX}" STREQUAL "")
	message(STATUS "Install directory not set")
else()
	message(STATUS "Install directory set to ${CMAKE_INSTALL_PREFIX}")
endif()

# install(
#   CODE
#   "
#   if (EXISTS \"\$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/man/man1/cadabra2.1\")
#      message(FATAL_ERROR \"\nThe location of the Cadabra manual has changed since the last installation. Please remove the old files with 'make deduplicate'.\n\")
#   endif()
#   "
#   )

# Turn Mathematica support on/off.
option(ENABLE_MATHEMATICA "Enable Mathematica support" OFF)

# Are we trying to build cadabra as a c++ library?
option(BUILD_AS_CPP_LIBRARY "Build cadabra as a C++ library" OFF)
if (BUILD_AS_CPP_LIBRARY)
  enable_testing()
  add_subdirectory(c++lib)
  configure_file(
	 "${PROJECT_SOURCE_DIR}/core/Config.hh.in"
	 "${PROJECT_SOURCE_DIR}/core/Config.hh"
    )
  # Bail out early.
  return()
endif()


# Include packaging logic.
include(cmake/packaging.cmake)

#---------------------------------------------------------------------------
# User options and other notifications
#---------------------------------------------------------------------------

#
option(MSVC_TARGET_CONSOLE "Force Release book on MSVC to display a console window" OFF)

# Provide option to build with Python 3 (default) or Python 2.
option(USE_PYTHON_3 "Use Python 3 if ON, or fall back to Python 2 if OFF" ON)

option(APPIMAGE_MODE "Run in AppImage mode, overriding path settings" OFF)
if(APPIMAGE_MODE)
  message(STATUS "Building for AppImage packaging (Debian paths, MicroTeX)")
  if("${CMAKE_INSTALL_PREFIX}" STREQUAL "/usr")
  else()
    MESSAGE(FATAL_ERROR "Building with -DAPPIMAGE_MODE=ON also requires -DCMAKE_INSTALL_PREFIX=/usr")
  endif()
endif()

option(PACKAGING_MODE "Run in packaging mode, overriding path settings" OFF)
if (PACKAGING_MODE)
	message(STATUS "Building in packaging mode")
	if("${CMAKE_INSTALL_PREFIX}" STREQUAL "/usr")
	else()
		MESSAGE(FATAL_ERROR "Building with -DPACKAGING_MODE=ON also requires -DCMAKE_INSTALL_PREFIX=/usr")
	endif()
else()
	message(STATUS "Building in user mode")
endif()

option(ENABLE_JUPYTER    "Enable building the Xeus-based Jupyter kernel" OFF)
option(ENABLE_PY_JUPYTER "Enable building the default Jupyter kernel"    ON)

if(ENABLE_JUPYTER)
  # Currently only possible when building against Conda.
  set(CONDA_FOUND TRUE)
else()
   set(CONDA_FOUND FALSE)
endif()

option(BUILD_TESTS "Build tests" ON)
if (BUILD_TESTS)
  message(STATUS "Building tests")
  # Allows tests to be built in all subdirectories.
  enable_testing()
endif()

option(ENABLE_FRONTEND    "Enable the UI frontend" ON)

option(ENABLE_SYSTEM_JSONCPP "Use the system-provided jsoncpp library" OFF)

option(INSTALL_TARGETS_ONLY "Only install targets; skipping icons, shared libraries etc..." OFF)
if (INSTALL_TARGETS_ONLY)
	message(STATUS "INSTALL_TARGETS_ONLY is enabled, please make sure all auxillary files and programs Cadabra requires are already installed")
endif()

#---------------------------------------------------------------------------
# Compiler flags.
#---------------------------------------------------------------------------

# - Set the C++ standard to C++14
# - Turn optimizations on
# - Turn off warnings we don't need

include(CheckIPOSupported)
check_ipo_supported(RESULT IPO_SUPPORTED OUTPUT error)
if( IPO_SUPPORTED )
    message(STATUS "IPO / LTO enabled")
else()
    message(STATUS "IPO / LTO not supported: <${error}>")
endif()



# GCC
if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
	if (ENABLE_FRONTEND)
		if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.9)
			message(FATAL_ERROR "GCC version must be at least 4.9 for regex support! See http://askubuntu.com/questions/428198/getting-installing-gcc-g-4-9-on-ubuntu and then set the environment variables CXX to g++-4.9 and CC to gcc-4.9. You may have to erase the build directory before re-running cmake.")
		endif()
	endif()
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
	set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
	# Disable specific warnings
	set(MSVC_FLAGS
		"/wd4250"						# inherits via dominance (rampant in the GTKMM codebase)
		"/wd4101"						# unreferenced local variable
		"/wd4244"						# conversion from x to y, possible loss of data
		"/wd4267"						# same as 4244
		"/wd4305"						# truncation from '' to 'char'
		"/wd4309"						# truncation of constant value
		"/wd4390"						# empty control statement, due to a DEBUG macro which requires trailing ;
		"/wd4996"						# deprecated POSIX functions
		"-D_CRT_SECURE_NO_WARNINGS"		# don't warn about deprecated functions
		"-D_SCL_SECURE_NO_WARNINGS"		# don't warn about unsafe function calls (e.g. std::copy with raw pointers)
		"-DNOMINMAX"					# prevent windows headers from defining min and max macros
		"-DWIN32_LEAN_AND_MEAN"			# stop windows from including a bunch of garbage
		"-DBOOST_ALL_DYN_LINK"			# ensure boost's auto-linking is enabled
	)
	foreach(FLAG ${MSVC_FLAGS})
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${FLAG}")
	endforeach()
endif()


#---------------------------------------------------------------------------
# Configure the various parts of Cadabra.
#---------------------------------------------------------------------------

# if(MATHEMATICA_FOUND)
#    # To avoid issues finding Mathematica's libWSTP64i4,
#    # when linking to Mathematica we set the RPATH.
#    # That's not something we want to do in general, as e.g. Debian's
#    # packages are not supposed to set RPATH.
#    SET(CMAKE_SKIP_BUILD_RPATH  FALSE)
#    SET(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
#    SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
#    SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
# endif()

configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/config/postinst.in"
	"${CMAKE_CURRENT_BINARY_DIR}/postinst"
	@ONLY
)
configure_file(
   "${CMAKE_CURRENT_SOURCE_DIR}/config/install_script.iss.in"
   "${CMAKE_CURRENT_SOURCE_DIR}/config/install_script.iss"
   )
configure_file(
   "${CMAKE_CURRENT_SOURCE_DIR}/config/pre_install.rtf.in"
   "${CMAKE_CURRENT_SOURCE_DIR}/config/pre_install.rtf"
   )


#---------------------------------------------------------------------------
# Configure Mathematica (if enabled).
#---------------------------------------------------------------------------

if(ENABLE_MATHEMATICA)
   print_header("Configuring Mathematica")
   cmake_policy(SET CMP0077 NEW)
   set(Mathematica_USE_STATIC_LIBRARIES TRUE)
   find_package(Mathematica COMPONENTS WSTP)
endif()


#---------------------------------------------------------------------------
# Configure Python.
#---------------------------------------------------------------------------

print_header("Configuring Python")

# NEEDED TO USE CMAKE_INSTALL_FULL<dir>
include(GNUInstallDirs)

if(USE_PYTHON_3)
	set(PYTHON_POSTFIX "3")
	message(STATUS "Building for use with Python 3 (good!)")
else()
	set(PYTHON_POSTFIX "")
	message(STATUS "Building for use with Python 2 (consider upgrading!)")
endif()

# set(PYBIND11_FINDPYTHON ON)
add_subdirectory(libs/pybind11)

message(STATUS "Found python ${PYTHON_LIBRARIES}")
#find_package (Python COMPONENTS Interpreter)

message(STATUS "Python version is ${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}.")
# We used to install in a different location; let's remove files from there
# if we find them on install.
# set(OLD_PYTHON_SITE_PATH ${CMAKE_INSTALL_FULL_LIBDIR}/python${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}/site-packages)

# The PYTHON_SITE_PATH variable is used throughout for installation purposes.
if(PACKAGING_MODE AND IS_DEBIAN_PACKAGE)
  # Debian packages install all their Python things in 'dist-packages', not 'site-packages'.
  set(PYTHON_SITE_PATH ${CMAKE_INSTALL_PREFIX}/lib/python${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}/dist-packages)
  # set(PYTHON_SITE_DIST "dist-packages")
elseif(APPIMAGE_MODE)
  set(PYTHON_SITE_PATH ${CMAKE_INSTALL_PREFIX}/lib/python${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}/dist-packages)
  set(PYTHON_SITE_DIST "dist-packages")
else()
  # The builder can override the path by setting it externally.
  if(NOT DEFINED PYTHON_SITE_PATH)
    # For everyone else there is 'site-packages' which we get from
    # calling into python's 'site' package (and hoping that the 0th
    # element is where we should be writing).
    execute_process(
      COMMAND ${PYTHON_EXECUTABLE} -c "import site; print(site.getsitepackages()[0])"
      OUTPUT_VARIABLE PYTHON_SITE_PATH
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    #   set(PYTHON_SITE_PATH ${CMAKE_INSTALL_PREFIX}/lib/python${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}/site-packages)
    # set(PYTHON_SITE_DIST "site-packages")
  endif()
endif()
message(STATUS "Installing Python modules in ${PYTHON_SITE_PATH}")
# We need to give our Python module an abi-name extension
# so that it can be installed in a folder which does not
# contain the abi name. See
# https://www.python.org/dev/peps/pep-3149/
execute_process(
	COMMAND ${PYTHON_EXECUTABLE} -c "import sysconfig; print(sysconfig.get_config_var('SOABI'))"
	OUTPUT_VARIABLE PYTHON_SOABI
	OUTPUT_STRIP_TRAILING_WHITESPACE
	)
message(STATUS "Python abi name ${PYTHON_SOABI}")

string(REGEX REPLACE "^${CMAKE_INSTALL_PREFIX}" "" PYTHON_SITE_PATH_REL ${PYTHON_SITE_PATH})
set(PYTHON_SITE_PATH_REL ${PYTHON_SITE_PATH_REL})

if(NOT WIN32)
  execute_process(
	 COMMAND ${PYTHON_EXECUTABLE} -c "import site; print (site.getsitepackages()[0]);"
	 OUTPUT_VARIABLE OLDER_PYTHON_SITE_PATH
	 OUTPUT_STRIP_TRAILING_WHITESPACE
	 )
endif()

# Suffixes
if(WIN32)
	set(STATIC_LIB_SUFFIX "lib")
	set(SHARED_LIB_SUFFIX "dll")
	set(PYTHON_MOD_SUFFIX "pyd")
else()
	set(STATIC_LIB_SUFFIX "a")
	set(SHARED_LIB_SUFFIX "so")
	set(PYTHON_MOD_EXT    "so")   
	set(PYTHON_MOD_SUFFIX "${PYTHON_SOABI}.so")
endif()

message(STATUS "Python module extension ${PYTHON_MOD_SUFFIX}")


#---------------------------------------------------------------------------
# Add subdirectories to project.
#---------------------------------------------------------------------------

# Jupyter kernel
print_header("Configuring Jupyter kernel")
if(ENABLE_JUPYTER)
  message(STATUS "Building the Xeus-based Jupyter kernel")
  # Currently only possible when building against Conda.
  set(CONDA_FOUND TRUE)
else()
   set(CONDA_FOUND FALSE)
   if(ENABLE_PY_JUPYTER)
      message(STATUS "Building the default Jupyter kernel")
   else()
      message(STATUS "Not building a Jupyter kernel")
   endif()
endif()
if(ENABLE_PY_JUPYTER)
   add_subdirectory(jupyterkernel)
endif()

# Core/packages
add_subdirectory(client_server)
add_subdirectory(core)

# Frontend
if(ENABLE_FRONTEND)
  option(ENABLE_MICROTEX "Enable MicroTeX support" OFF)
  if(APPIMAGE_MODE)
    set(ENABLE_MICROTEX TRUE)
  endif()
  if(ENABLE_MICROTEX)
    set(USE_MICROTEX TRUE)
    set(tinyxml2_BUILD_TESTING FALSE)
    add_subdirectory(libs/tinyxml2)
    add_subdirectory(libs/microtex EXCLUDE_FROM_ALL)
    target_compile_options(LaTeX PUBLIC "-Wno-reorder" "-Wno-sign-compare" "-Wno-switch" "-Wno-unused-variable")
  endif()
  add_subdirectory(frontend)
  if(ENABLE_MICROTEX)
    add_dependencies(cadabra2-gtk LaTeX)
  endif()
endif()

# Tests
if(BUILD_TESTS)
  add_subdirectory(tests)
endif()

add_subdirectory(web2 EXCLUDE_FROM_ALL)

# Generate the core/Config.hh file; this needs to come as late as possible
# in this CMakeLists.txt to ensure that all variables have been set.
configure_file(
	"${PROJECT_SOURCE_DIR}/core/Config.hh.in"
	"${PROJECT_SOURCE_DIR}/core/Config.hh"
)

#---------------------------------------------------------------------------
# Provide uninstall target.
#---------------------------------------------------------------------------

configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
	"${CMAKE_CURRENT_BINARY_DIR}/cmake/cmake_uninstall.cmake"
	IMMEDIATE @ONLY
)

add_custom_target(uninstall
	"${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake/cmake_uninstall.cmake"
)

#---------------------------------------------------------------------------
# Provide target to build AppImage.
#---------------------------------------------------------------------------

if(APPIMAGE_MODE)
  set(DEPLOY      linuxdeploy-${CMAKE_SYSTEM_PROCESSOR}.AppImage)
  set(DEPLOY_URL  https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/${DEPLOY})
  message(STATUS "Generating AppImage using ${DEPLOY_URL}")
  if(NOT EXISTS ${DEPLOY})
    message(STATUS "Downloading...")
    file(DOWNLOAD ${DEPLOY_URL} ${DEPLOY})
    execute_process(COMMAND chmod u+x ${DEPLOY})
  endif()
  add_custom_target(appimage
    COMMAND ./${DEPLOY} --appdir AppDir --output appimage
  )
endif()

print_header("All scripts completed")
