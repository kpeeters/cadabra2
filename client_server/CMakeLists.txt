cmake_minimum_required(VERSION 2.8)

project(CadabraServer)

if (POLICY CMP0042)
  cmake_policy(SET CMP0042 NEW)
endif(POLICY CMP0042)

# Now using boost::regex, so we can get away with gcc 4.8.
#
# if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
#     if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.9)
#         message(FATAL_ERROR "GCC version must be at least 4.9 for regex support! See http://askubuntu.com/questions/428198/getting-installing-gcc-g-4-9-on-ubuntu and then set the environment variables CXX to g++-4.9 and CC to gcc-4.9. You may have to erase the build directory before re-running cmake.")
#     endif()
# endif()

# Set path to additional cmake files
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/../cmake/modules")

# Locate python

if(USE_PYTHON_3)
  set(Python_ADDITIONAL_VERSIONS 3.4 3.5 3.6)
  set(PythonInterp_ADDITIONAL_VERSIONS 3.4 3.5 3.6)
  find_package(PythonInterp 3.4 REQUIRED)
  if(APPLE)
     find_package(PythonLibsOSX 3.4 REQUIRED)
  else()
     find_package(PythonLibs 3.4 REQUIRED)
  endif()
else()
  set(Python_ADDITIONAL_VERSIONS 2.7)
  set(PythonInterp_ADDITIONAL_VERSIONS 2.7)
  find_package(PythonInterp 2.7 REQUIRED)
  if(APPLE)
     find_package(PythonLibsOSX 2.7 REQUIRED)
  else()
     find_package(PythonLibs 2.7 REQUIRED)
  endif()
endif()
message("-- Found python ${PYTHON_LIBRARIES}")

# Locate glibmm (for spawning processes)
if(WIN32)
	message("-- Warning: Windows build skipped: glibmm")
	# Ended up just factoring it out in favor of windows calls as glibmm seemed like a huge pain to get going in leu of createprocess
	# find_package(GLIBMM REQUIRED)
	# include_directories(${GLIBMM3_INCLUDE_DIRS})
	# link_directories(${GLIBMM3_LIBRARY_DIRS})
else()
	find_package(PkgConfig REQUIRED)
	pkg_check_modules(GLIBMM3 REQUIRED glibmm-2.4)
	include_directories(${GLIBMM3_INCLUDE_DIRS})
	link_directories(${GLIBMM3_LIBRARY_DIRS})
	add_definitions(${GLIBMM3_CFLAGS_OTHER})
endif()

find_package(SQLite3 REQUIRED)

# Locate Boost
add_definitions("-DBOOST_ALL_NO_LIB")
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
if(USE_PYTHON_3)
  find_package(Boost 1.53.0 COMPONENTS python3)
  if(Boost_PYTHON3_FOUND)
    find_package(Boost 1.53.0 COMPONENTS python3 system regex program_options date_time filesystem REQUIRED)
  else()
    if("${PYTHONLIBS_VERSION_STRING}" MATCHES "^3.4")
      find_package(Boost 1.53.0 COMPONENTS python-py34 system regex program_options date_time filesystem REQUIRED)
    else()
		if("${PYTHONLIBS_VERSION_STRING}" MATCHES "^3.5")    
		  find_package(Boost 1.53.0 COMPONENTS python-py35 system regex program_options date_time filesystem REQUIRED)
		else()
		  find_package(Boost 1.53.0 COMPONENTS python-py36 system regex program_options date_time filesystem REQUIRED)
		endif()
    endif()
  endif()
else()
  find_package(Boost 1.53.0 COMPONENTS python system regex program_options date_time filesystem REQUIRED)
endif()
include_directories("${Boost_INCLUDE_DIRS}" ${PYTHON_INCLUDE_DIR} ${SQLITE3_INCLUDE_DIR} ../core)

if(WIN32)
	message("-- Warning: Windows build skipped: pkgconfig glibmm")
else()
	find_package(PkgConfig REQUIRED)
endif()
#pkg_check_modules(UUID uuid REQUIRED)
#include_directories("${UUID_INCLUDE_DIRS}")

# Snoop for logging. At the moment the required Snoop.cc|.hh are included
# in the Cadabra sources so we do not need to look for this. However, we
# do need to find sqlite3 for this to work.

# find_package(Snoop REQUIRED)

# Locate gmpxx. On Homebrew there seems to be constantly something wrong with
# the pkgconfig for gmpxx. So we just add the include path by hand.
if(APPLE)
	add_definitions("-I/usr/local/include -I/opt/local/include")
    find_library(GMP_LIB gmp REQUIRED)
	find_library(GMPXX_LIB gmpxx REQUIRED)
elseif(WIN32)
	# by default in windows the gmp library puts results directly on the drive here
    add_definitions("-I/msvc/include/ -I/msvc/lib/x86")
    message("--- Adding ""-I/msvc/include -I/msvc/lib/x86"" for WIN32 for GMP")
	
	find_library(GMP_LIB NAMES gmp HINTS /msvc/lib/x86/ /msvc/include/)
	find_library(GMPXX_LIB NAMES gmpxx HINTS /msvc/lib/x86/ /msvc/include/)
else()
	find_library(GMP_LIB gmp REQUIRED)
	find_library(GMPXX_LIB gmpxx REQUIRED)
endif()
message("-- Found gmp ${GMP_LIB}")
message("-- Found gmpxx ${GMPXX_LIB}")
message("-- Found python ${PYTHON_LIBRARIES}")
include_directories("${Boost_INCLUDE_DIRS}" ${PYTHON_INCLUDE_DIRS})
message("-- Python include dirs: ${PYTHON_INCLUDE_DIRS}")

if(CMAKE_COMPILER_IS_GNUCXX)
  add_definitions("-Wall -g -Wno-unused-but-set-variable")
endif()
add_definitions("-D_WEBSOCKETPP_CPP11_STL_ -DBOOST_ASIO_HAS_STD_CHRONO")

# Create the server library
#link_directories(${UUID_LIBRARY_DIRS})
add_library(cadabra_server SHARED Server.cc Snoop.cc jsoncpp/jsoncpp.cpp ../core/CdbPython.cc ../core/Stopwatch.cc ../core/ProgressMonitor.cc)
target_link_libraries(cadabra_server ${Boost_LIBRARIES} ${SNOOP_LIBRARIES} ${SQLITE3_LIBRARIES} ${PYTHON_LIBRARIES} ${GLIBMM3_LIBRARIES} ${GMPXX_LIB} ${GMP_LIB})
if (WIN32)
   target_link_libraries(cadabra_server ws2_32 mswsock)
endif()

# Create the client library.
add_library(cadabra_client SHARED ComputeThread.cc DocumentThread.cc DataCell.cc Actions.cc Snoop.cc jsoncpp/jsoncpp.cpp)
target_link_libraries(cadabra_client ${Boost_LIBRARIES} ${SQLITE3_LIBRARIES} ${PYTHON_LIBRARIES} ${GLIBMM3_LIBRARIES})
if (WIN32)
   target_link_libraries(cadabra_client ws2_32)
endif()

# Create server binary
include_directories("." "./websocketpp" "./jsoncpp")
add_executable(cadabra-server cadabra-server.cc)
add_executable(cadabra2html cadabra2html.cc DataCell.cc)
add_executable(cadabra2cadabra cadabra2cadabra.cc DataCell.cc)
#add_executable(test_talk_to_server test_talk_to_server.cc)
#add_executable(regexp_tester regexp_tester.cc)
target_link_libraries(cadabra-server cadabra_server)
target_link_libraries(cadabra2html cadabra_client)
target_link_libraries(cadabra2cadabra cadabra_client)
#target_link_libraries(test_talk_to_server ${Boost_LIBRARIES} -lpthread)

# The preamble 'notebook.tex' should be kept in sync with the one in
# ../frontend/common/texengine; this one is meant for printing so
# slightly different, but cadabra-specific macros should be copied.
set(INSTALL_LATEX_DIR "${CMAKE_INSTALL_PREFIX}/share/cadabra2")
install(FILES  notebook.tex  DESTINATION ${INSTALL_LATEX_DIR})
install(FILES  notebook.html DESTINATION ${INSTALL_LATEX_DIR})

#set(INSTALL_LIB_DIR     lib           CACHE PATH "Installation directory for libraries")
install(TARGETS cadabra-server DESTINATION bin)
install(TARGETS cadabra2html DESTINATION bin)
install(TARGETS cadabra_client LIBRARY ARCHIVE DESTINATION lib LIBRARY DESTINATION lib)
install(TARGETS cadabra_server LIBRARY ARCHIVE DESTINATION lib LIBRARY DESTINATION lib)
