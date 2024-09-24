set(CMAKE_LEGACY_CYGWIN_WIN32 0)
set(CADABRA_CMAKE_VERSION 3.12)

# Policy settings for CMake to resolve ambiguities.

if(POLICY CMP0042)
  cmake_policy(SET CMP0042 NEW)
  message(STATUS "Set CMake policy CM0042 to NEW")
endif()
if(POLICY CMP0054)
  cmake_policy(SET CMP0054 NEW)
  message(STATUS "Set CMake policy CM0054 to NEW")
endif()
if(POLICY CMP0127)
  cmake_policy(SET CMP0127 NEW)
  message(STATUS "Set CMake policy CM0127 to NEW")
endif()
if(POLICY CMP0148)
  cmake_policy(SET CMP0148 NEW)
  message(STATUS "Set CMake policy CM0148 to NEW")
endif()
if(POLICY CMP0167)
  cmake_policy(SET CMP0167 NEW)
  message(STATUS "Set CMake policy CM0167 to NEW")
endif()

cmake_minimum_required(VERSION ${CADABRA_CMAKE_VERSION})
set(CMAKE_CXX_STANDARD 17)
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
message(STATUS "Architecture is '${CMAKE_SYSTEM_PROCESSOR}'")

# Store the version number in a build/VERSION file (so that e.g. github
# actions can pick it up).
if(NOT "${CADABRA_VERSION_TWEAK}" STREQUAL "0")
  file(WRITE build/VERSION       "${CADABRA_VERSION_MAJOR}.${CADABRA_VERSION_MINOR}.${CADABRA_VERSION_PATCH}.${CADABRA_VERSION_TWEAK}")
else()
  file(WRITE build/VERSION       "${CADABRA_VERSION_MAJOR}.${CADABRA_VERSION_MINOR}.${CADABRA_VERSION_PATCH}")
endif()  


# Notify about install directory
if ("${CMAKE_INSTALL_PREFIX}" STREQUAL "")
	message(STATUS "Install directory not set")
else()
	message(STATUS "Install directory set to ${CMAKE_INSTALL_PREFIX}")
endif()

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
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O2 -Wall -Wextra -Wunused -Wno-template-id-cdtor -Wno-unknown-pragmas -Wno-misleading-indentation -fvisibility=hidden -Wno-unused-but-set-variable -Wno-unused-parameter")
endif()

# Clang
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  # For Clang, need to additionally check version to avoid compiler bugs
  if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 3.5)
    message(FATAL_ERROR "Clang version must be at least 3.5 to avoid known bugs.")
  endif()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O2 -fvisibility=hidden -Wall -Wextra -Wunused -Wno-unused-parameter -Wno-null-pointer-subtraction")
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
configure_file(
   "${CMAKE_CURRENT_SOURCE_DIR}/config/science.cadabra.cadabra2-gtk.desktop.in"
   "${CMAKE_CURRENT_SOURCE_DIR}/config/science.cadabra.cadabra2-gtk.desktop"
   )
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/frontend/gtkmm/science.cadabra.cadabra2-gtk.appdata.xml.in"
  "${CMAKE_CURRENT_SOURCE_DIR}/frontend/gtkmm/science.cadabra.cadabra2-gtk.appdata.xml"  
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

set(Python_POSTFIX "3")
find_package(Python COMPONENTS Interpreter Development)
find_package(pybind11 CONFIG)
if (NOT pybind11_FOUND)
  message(STATUS "Using included pybind11.")
  add_subdirectory(libs/pybind11)
endif()

message(STATUS "Found python ${Python_LIBRARIES}")
message(STATUS "Python version is ${Python_VERSION_MAJOR}.${Python_VERSION_MINOR}.")

# The PYTHON_SITE_PATH variable is used for installation purposes
# only. It is ideally a path relative to CMAKE_INSTALL_PREFIX, not an
# absolute path which uses this variable explictly. See the CMake docs
# for `install`.
if(WIN32)
  set(PYTHON_SITE_PATH lib/python${Python_VERSION_MAJOR}.${Python_VERSION_MINOR}/site-packages)
  # NOTE: if you change the CDB_BIN_PATH, you also need to change the stripping
  # logic in InstallPrefix.cc. Note that CDB_BIN_PATH needs to be a relative path.
  set(CDB_BIN_PATH .)
else()
  set(CDB_BIN_PATH bin)
  if(PACKAGING_MODE AND IS_DEBIAN_PACKAGE)
    # Debian packages install all their Python things in 'dist-packages', not 'site-packages'.
    set(PYTHON_SITE_PATH lib/python${Python_VERSION_MAJOR}.${Python_VERSION_MINOR}/dist-packages)
    # set(Python_SITE_DIST "dist-packages")
  elseif(APPIMAGE_MODE)
    set(PYTHON_SITE_PATH lib/python${Python_VERSION_MAJOR}.${Python_VERSION_MINOR}/dist-packages)
    set(Python_SITE_DIST "dist-packages")
  else()
    # The builder can override the path by setting it externally.
    if(NOT DEFINED PYTHON_SITE_PATH)
      # set(PYTHON_SITE_PATH ${Python_SITEARCH})
      # For everyone else there is 'site-packages' which we get from
      # calling into python's 'site' package (and hoping that the 0th
      # element is where we should be writing).
      execute_process(
	COMMAND ${Python_EXECUTABLE} -c "import site; print(site.getsitepackages()[0])"
	OUTPUT_VARIABLE PYTHON_SITE_PATH
	OUTPUT_STRIP_TRAILING_WHITESPACE
      )
    endif()
  endif()
endif()
if(IS_ABSOLUTE ${PYTHON_SITE_PATH})
  message(STATUS "Making PYTHON_SITE_PATH relative if possible")
  string(REPLACE "${CMAKE_INSTALL_PREFIX}/" "" PYTHON_SITE_PATH_REL "${PYTHON_SITE_PATH}")
  set(PYTHON_SITE_PATH ${PYTHON_SITE_PATH_REL})
else()
  message(STATUS "PYTHON_SITE_PATH is already relative")
endif()

message(STATUS "Installing Cadabra Python modules in (PYTHON_SITE_PATH) ${PYTHON_SITE_PATH}")
message(STATUS "relative to CMake install prefix ${CMAKE_INSTALL_PREFIX}")
# message(STATUS "Python standard  platform-independent installation directory (Python_STDLIB)   ${Python_STDLIB}")
# message(STATUS "Python standard  platform-dependent   installation directory (Python_STDARCH)  ${Python_STDARCH}")
# message(STATUS "Python 3rd-party platform-independent installation directory (Python_SITELIB)  ${Python_SITELIB}")
# message(STATUS "Python 3rd-party platform-dependent   installation directory (Python_SITEARCH) ${Python_SITEARCH}")
# We need to give our Python module an abi-name extension
# so that it can be installed in a folder which does not
# contain the abi name. See
# https://www.python.org/dev/peps/pep-3149/
execute_process(
	COMMAND ${Python_EXECUTABLE} -c "import sysconfig; print(sysconfig.get_config_var('SOABI'))"
	OUTPUT_VARIABLE Python_SOABI
	OUTPUT_STRIP_TRAILING_WHITESPACE
	)
message(STATUS "Python abi name ${Python_SOABI}")

# Suffixes
if(WIN32)
	set(STATIC_LIB_SUFFIX "lib")
	set(SHARED_LIB_SUFFIX "dll")
	set(Python_MOD_SUFFIX "pyd")
	set(CMAKE_SHARED_LIBRARY_SUFFIX ".dll")
	set(CMAKE_FIND_LIBRARY_SUFFIXES ".dll" ".a")
else()
	set(STATIC_LIB_SUFFIX "a")
	set(SHARED_LIB_SUFFIX "so")
	set(Python_MOD_EXT    "so")   
	set(Python_MOD_SUFFIX "${Python_SOABI}.so")
endif()

message(STATUS "Python module extension ${Python_MOD_SUFFIX}")


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
  set(ENABLE_MICROTEX TRUE)
  if(ENABLE_MICROTEX)
    set(USE_MICROTEX TRUE)
    set(tinyxml2_BUILD_TESTING FALSE)
  endif()
  add_subdirectory(frontend)
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

# Some additional logic to install all runtime dependencies of our binaries
# into the target installation directory on windows. 
if(WIN32)
  set(EXECUTABLES
    core/cadabra2-cli
    core/cdb-nbtool
    client_server/cadabra-server
    frontend/gtkmm/cadabra2-gtk
  )
  list(JOIN EXECUTABLES " " LEXECUTABLES)
  # Custom command to run ldd, get dependencies, and install these in a
  # folder ready to be processed by `install`. The 'ldd' command does not
  # run on the '*.pyd' file, but if we rename or copy it to have extension '.dll'
  # all goes through fine...
  add_custom_command(
    OUTPUT  ${CMAKE_BINARY_DIR}/ldd_dependencies.txt
    COMMAND cp core/cadabra2.pyd core/cadabra2.dll
    COMMAND bash -c "ldd core/cadabra2.dll ${LEXECUTABLES} | grep '=>' | sed -e 's/^[^=]*=>[ ]*\\([^ ]*\\).*/\\1/' > ${CMAKE_BINARY_DIR}/ldd_dependencies.txt"
    COMMAND rm core/cadabra2.dll
    COMMAND mkdir -p ${CMAKE_BINARY_DIR}/deps
    COMMAND bash -c "for f in `cat ${CMAKE_BINARY_DIR}/ldd_dependencies.txt`; do cp \${f} ${CMAKE_BINARY_DIR}/deps/; done"
    VERBATIM
    DEPENDS  core/cadabra2.pyd ${EXECUTABLES}
    COMMENT "Using ldd to copying dependencies of ${EXECUTABLES} to ${CMAKE_BINARY_DIR}/deps"
  )
  add_custom_target("LDD_deps" ALL echo -n DEPENDS ldd_dependencies.txt)
  
#  # Read ldd output and install dependencies
#  install(CODE [[
#    file(STRINGS "${CMAKE_SOURCE_DIR}/ldd_dependencies.txt" ldd_output)
#    foreach(line IN LISTS ldd_output)
#        message(STATUS "Considering ${line}")
#        # The ldd output format is like: libname => /path/to/libname (0x...)
#        string(REGEX MATCH "=> ([^ ]+)" match ${line})
#        if(match)
#            # message(STATUS "Found dependency: ${CMAKE_MATCH_1}")
#            execute_process(COMMAND cygpath -m ${CMAKE_MATCH_1} OUTPUT_VARIABLE WINPATH OUTPUT_STRIP_TRAILING_WHITESPACE)
#            # message(STATUS "Copying dependency: ${WINPATH}")
#            # Here we would like to use 'install', but that's not scriptable...
#            file(INSTALL ${WINPATH} DESTINATION ${CMAKE_INSTALL_PREFIX}/${CDB_BIN_PATH})
#        endif()
#    endforeach()
#  ]])

  # Install all the dependencies into the root destination folder.
  install(DIRECTORY deps/ DESTINATION . FILES_MATCHING PATTERN "*.dll")      

  # We need gdbus to setup the dbus, needed by Glib, otherwise anything
  # gtk-related will just bail out at start. We also need the helper
  # program to spawn programs using Glib.
  install(FILES C:/msys64/ucrt64/bin/gdbus.exe                       DESTINATION .)
  install(FILES C:/msys64/ucrt64/bin/gspawn-win64-helper.exe         DESTINATION .)
  install(FILES C:/msys64/ucrt64/bin/gspawn-win64-helper-console.exe DESTINATION .)
endif()
    
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
  # We need linuxdeploy to create the AppImage. It is best to have this built
  # from source and installed locally. Make sure to first install deps by doing
  #
  #   sudo apt install libcimg-dev libgtest-dev
  #
  # Then checkout appimagetool, linuxdeploy, linuxdeploy-plugin-appimage using
  #
  #   git clone https://github.com/AppImage/appimagetool.git --recurse-submodules
  #   git clone https://github.com/linuxdeploy/linuxdeploy.git --recurse-submodules
  #   git clone https://github.com/linuxdeploy/linuxdeploy-plugin-appimage.git --recurse-submodules
  #
  # Do *not* forget the `--recurse-submodules` flag!
  #
  # Build `appimagetool` as usual, and for the other two use
  #
  #   cd linuxdeploy && mkdir build && cd build
  #   cmake -DBUILD_TESTING=OFF ..
  #   make && sudo make install
  #
  # and ditto for the plugin,
  #
  #   cd linuxdeploy && mkdir build && cd build
  #   cmake -DBUILD_TESTING=OFF ..
  #   make && sudo make install
  #
  # If you do not have it locally, the configure process will download an
  # AppImage for linuxdeploy, but this does *not* run inside a QEMU container.
  
  print_header("Configuring AppImage build")
  find_program(LINUXDEPLOY linuxdeploy "${CMAKE_BINARY_DIR}")
  set(DEPLOY         linuxdeploy-${CMAKE_SYSTEM_PROCESSOR}.AppImage)
  set(DEPLOY_URL     https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/${DEPLOY})
  set(DEPLOY_GTK_URL https://raw.githubusercontent.com/linuxdeploy/linuxdeploy-plugin-gtk/master/linuxdeploy-plugin-gtk.sh)
  if(${LINUXDEPLOY} MATCHES "LINUXDEPLOY-NOTFOUND")
    message(STATUS "The 'linuxdeploy' binary is not present, downloading one")
    message(STATUS "Generating AppImage using ${DEPLOY_URL}")
    if(NOT EXISTS "linuxdeploy")
      message(STATUS "Downloading...")
      file(DOWNLOAD ${DEPLOY_URL} ${CMAKE_BINARY_DIR}/linuxdeploy)
      execute_process(COMMAND chmod u+x ${CMAKE_BINARY_DIR}/linuxdeploy)
    endif()
    set(LINUXDEPLOY ${CMAKE_BINARY_DIR}/linuxdeploy)
  else()
    message(STATUS "Found linuxdeploy at ${LINUXDEPLOY}")
  endif()
  if(NOT EXISTS "linuxdeploy-plugin-gtk.sh")
      file(DOWNLOAD ${DEPLOY_GTK_URL} ${CMAKE_BINARY_DIR}/linuxdeploy-plugin-gtk.sh)
      execute_process(COMMAND chmod u+x ${CMAKE_BINARY_DIR}/linuxdeploy-plugin-gtk.sh)
  endif()
  add_custom_target(appimage
    COMMAND mkdir -p AppDir/${Python_SITELIB}
    COMMAND cp -a ${CMAKE_SOURCE_DIR}/config/AppRun AppDir/
    COMMAND chmod gou+x ${CMAKE_SOURCE_DIR}/config/AppRun AppDir/AppRun
    COMMAND cp -a ${Python_SITELIB}/setuptools AppDir/${PYTHON_SITELIB}/
    COMMAND cp -a ${Python_STDARCH}/* AppDir/${Python_STDARCH}/
    COMMAND ${LINUXDEPLOY} --appdir AppDir --plugin gtk --desktop-file AppDir/usr/share/applications/science.cadabra.cadabra2-gtk.desktop --output appimage
    COMMAND mv Cadabra_2-${CMAKE_SYSTEM_PROCESSOR}.AppImage Cadabra_${CADABRA_VERSION_MAJOR}.${CADABRA_VERSION_MINOR}.${CADABRA_VERSION_PATCH}_${CMAKE_SYSTEM_PROCESSOR}.AppImage
  )
endif()

print_header("All scripts completed")
