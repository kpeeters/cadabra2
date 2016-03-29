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

# Locate Boost
find_package(Boost 1.45.0 COMPONENTS python system regex log program_options)
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(PkgConfig REQUIRED)
find_package(PythonLibs 2.7 REQUIRED)
#find_package(websocketpp REQUIRED)            # from github
pkg_check_modules(JSONCPP jsoncpp REQUIRED)   # libjsoncpp-dev
#find_library(JSONCPP_LIB jsoncpp REQUIRED)
#find_package(jsoncpp REQUIRED)
message("-- Found JSONCPP include dir: ${JSONCPP_INCLUDE_DIRS}")
message("-- Found JSONCPP library dir: ${JSONCPP_LIBRARY_DIRS}")
message("-- Found JSONCPP libraries  : ${JSONCPP_LIBRARIES}")
include_directories("${Boost_INCLUDE_DIRS}" ${PYTHON_INCLUDE_DIR})
if(JSONCPP_INCLUDE_DIRS)
include_directories("${JSONCPP_INCLUDE_DIRS}")
endif(JSONCPP_INCLUDE_DIRS)
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
link_directories(${UUID_LIBRARY_DIRS} ${JSONCPP_LIBRARY_DIRS})
add_library(cadabra_server SHARED Server.cc Snoop.cc)
target_link_libraries(cadabra_server ${Boost_LIBRARIES} ${PYTHON_LIBRARIES} ${JSONCPP_LIBRARIES} ${SNOOP_LIBRARIES} sqlite3 ${UUID_LIBRARIES})

# Create the client library. 
add_library(cadabra_client SHARED ComputeThread.cc DocumentThread.cc DataCell.cc Actions.cc popen2.cc Snoop.cc)
target_link_libraries(cadabra_client ${Boost_LIBRARIES} ${JSONCPP_LIBRARIES} sqlite3 ${UUID_LIBRARIES} ${PYTHON_LIBRARIES})

# Create server binary
#include_directories("." ${WEBSOCKETPP_INCLUDE_DIR} ${JSONCPP_INCLUDE_DIR})
include_directories("." "./websocketpp" ${JSONCPP_INCLUDE_DIR})
add_executable(cadabra-server cadabra-server.cc)
add_executable(cadabra2html cadabra2html.cc DataCell.cc)
#add_executable(test_talk_to_server test_talk_to_server.cc)
#add_executable(regexp_tester regexp_tester.cc)
target_link_libraries(cadabra-server cadabra_server ${Boost_LIBRARIES} ${PYTHON_LIBRARIES} ${JSONCPP_LIBRARIES})
target_link_libraries(cadabra2html ${JSONCPP_LIBRARIES} ${Boost_LIBRARIES} ${PYTHON_LIBRARIES})
#target_link_libraries(test_talk_to_server ${Boost_LIBRARIES} -lpthread)

set(INSTALL_LIB_DIR     lib           CACHE PATH "Installation directory for libraries")
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/cadabra-server DESTINATION bin)
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/cadabra2html DESTINATION bin)
install(TARGETS cadabra_client LIBRARY DESTINATION "${INSTALL_LIB_DIR}")
install(TARGETS cadabra_server LIBRARY DESTINATION "${INSTALL_LIB_DIR}")


