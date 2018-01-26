cmake_minimum_required(VERSION 3.1)
project(Cadabra)
message("------------------------------------------------------------------------\nConfiguring 'client_server'")

# Now using boost::regex, so we can get away with gcc 4.8.
#
# if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
#     if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.9)
#         message(FATAL_ERROR "GCC version must be at least 4.9 for regex support! See http://askubuntu.com/questions/428198/getting-installing-gcc-g-4-9-on-ubuntu and then set the environment variables CXX to g++-4.9 and CC to gcc-4.9. You may have to erase the build directory before re-running cmake.")
#     endif()
# endif()

# Set path to additional cmake files
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/../cmake/modules")

# Locate glibmm (for spawning processes)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GLIBMM3 REQUIRED glibmm-2.4)
include_directories(${GLIBMM3_INCLUDE_DIRS})
link_directories(${GLIBMM3_LIBRARY_DIRS})
add_definitions(${GLIBMM3_CFLAGS_OTHER})

# Locate sqlite3.
find_package(SQLite3)

# Locate Boost
find_package(Boost 1.53.0 COMPONENTS system regex program_options date_time filesystem REQUIRED)
find_package(PkgConfig REQUIRED)
include_directories("${Boost_INCLUDE_DIRS}" ${PYTHON_INCLUDE_DIR} ${SQLITE3_INCLUDE_DIR} ../core)
#pkg_check_modules(UUID uuid REQUIRED)
#include_directories("${UUID_INCLUDE_DIRS}")
include_directories(".")

# Snoop for logging. At the moment the required Snoop.cc|.hh are included
# in the Cadabra sources so we do not need to look for this. However, we
# do need to find sqlite3 for this to work.

# find_package(Snoop REQUIRED)


if(CMAKE_COMPILER_IS_GNUCXX)
  add_definitions("-Wall -g -Wno-unused-but-set-variable")
endif()
add_definitions("-D_WEBSOCKETPP_CPP11_STL_ -DBOOST_ASIO_HAS_STD_CHRONO")

# Create the server library
#link_directories(${UUID_LIBRARY_DIRS})
add_library(cadabra_server SHARED Server.cc Snoop.cc jsoncpp/jsoncpp.cpp ../core/CdbPython.cc ../core/Stopwatch.cc ../core/ProgressMonitor.cc)
target_link_libraries(cadabra_server ${Boost_LIBRARIES} ${SNOOP_LIBRARIES} ${SQLITE3_LIBRARIES} ${PYTHON_LIBRARIES} ${GLIBMM3_LIBRARIES})
target_link_libraries(cadabra_server pybind11::embed)
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
