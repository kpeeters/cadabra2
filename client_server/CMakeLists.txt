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

set(Python_ADDITIONAL_VERSIONS 3.4)
if(APPLE)
   find_package(PythonLibsOSX 3.4 REQUIRED)
else()
   find_package(PythonLibs 3.4 REQUIRED)
endif()
find_package(PythonInterp 3.4 REQUIRED)

# Locate Boost
find_package(Boost 1.54.0 COMPONENTS python system regex program_options REQUIRED)
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(PkgConfig REQUIRED)
include_directories("${Boost_INCLUDE_DIRS}" ${PYTHON_INCLUDE_DIR})
pkg_check_modules(UUID uuid REQUIRED)
include_directories("${UUID_INCLUDE_DIRS}")

# Snoop for logging. At the moment the required Snoop.cc|.hh are included
# in the Cadabra sources so we do not need to look for this. However, we
# do need to find sqlite3 for this to work.

# find_package(Snoop REQUIRED)
find_library(sqlite3 sqlite3 required)


if(CMAKE_COMPILER_IS_GNUCXX)
  add_definitions("-Wall -g -Wno-unused-but-set-variable")
endif()
add_definitions("-D_WEBSOCKETPP_CPP11_STL_ -DBOOST_ASIO_HAS_STD_CHRONO")

# Create the server library
link_directories(${UUID_LIBRARY_DIRS})
add_library(cadabra_server SHARED Server.cc Snoop.cc jsoncpp/jsoncpp.cpp)
target_link_libraries(cadabra_server ${SNOOP_LIBRARIES} sqlite3 ${UUID_LIBRARIES} ${Boost_LIBRARIES} ${PYTHON_LIBRARIES} )

# Create the client library. 
add_library(cadabra_client SHARED ComputeThread.cc DocumentThread.cc DataCell.cc Actions.cc popen2.cc Snoop.cc jsoncpp/jsoncpp.cpp)
target_link_libraries(cadabra_client ${Boost_LIBRARIES} sqlite3 ${UUID_LIBRARIES} ${PYTHON_LIBRARIES})

# Create server binary
include_directories("." "./websocketpp" "./jsoncpp")
add_executable(cadabra-server cadabra-server.cc)
add_executable(cadabra2html cadabra2html.cc DataCell.cc)
#add_executable(test_talk_to_server test_talk_to_server.cc)
#add_executable(regexp_tester regexp_tester.cc)
target_link_libraries(cadabra-server cadabra_server ${Boost_LIBRARIES} ${PYTHON_LIBRARIES})
target_link_libraries(cadabra2html cadabra_client ${Boost_LIBRARIES} ${PYTHON_LIBRARIES})
#target_link_libraries(test_talk_to_server ${Boost_LIBRARIES} -lpthread)

set(INSTALL_LIB_DIR     lib           CACHE PATH "Installation directory for libraries")
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/cadabra-server DESTINATION bin)
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/cadabra2html DESTINATION bin)
install(TARGETS cadabra_client LIBRARY DESTINATION "${INSTALL_LIB_DIR}")
install(TARGETS cadabra_server LIBRARY DESTINATION "${INSTALL_LIB_DIR}")


