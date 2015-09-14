cmake_minimum_required(VERSION 2.8)

project(CadabraServer)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.9)
        message(FATAL_ERROR "GCC version must be at least 4.9 for regex support! See http://askubuntu.com/questions/428198/getting-installing-gcc-g-4-9-on-ubuntu and then set the environment variables CXX to g++-4.9 and CC to gcc-4.9. You may have to erase the build directory before re-running cmake.")
    endif()
endif()

# Set path to additional cmake files
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/../cmake/modules")

# Locate Boost
find_package(Boost 1.45.0 COMPONENTS python system regex log)
include_directories("${Boost_INCLUDE_DIRS}" ${PYTHON_INCLUDE_DIR})
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(PkgConfig REQUIRED)
find_package(PythonLibs 2.7 REQUIRED)
#find_package(websocketpp REQUIRED)            # from github
#pkg_check_modules(JSONCPP jsoncpp REQUIRED)   # libjsoncpp-dev
find_library(JSONCPP_LIB jsoncpp REQUIRED)
message("-- Found JSONCPP ${JSONCPP_LIB}")

# Snoop for logging
find_package(Snoop REQUIRED)

if(CMAKE_COMPILER_IS_GNUCXX)
  add_definitions("-Wall -g -Wno-unused-but-set-variable")
endif()
add_definitions("-D_WEBSOCKETPP_CPP11_STL_")

# Create the server library
add_library(cadabra_server SHARED Server.cc)
target_link_libraries(cadabra_server ${Boost_LIBRARIES} ${PYTHON_LIBRARIES} ${JSONCPP_LIB} ${SNOOP_LIBRARIES} uuid)

# Create the client library. 
add_library(cadabra_client SHARED ComputeThread.cc DocumentThread.cc DataCell.cc Actions.cc popen2.cc)
target_link_libraries(cadabra_client ${Boost_LIBRARIES} ${JSONCPP_LIB})

# Create server binary
#include_directories("." ${WEBSOCKETPP_INCLUDE_DIR} ${JSONCPP_INCLUDE_DIR})
include_directories("." "./websocketpp" ${JSONCPP_INCLUDE_DIR})
add_executable(cadabra-server cadabra-server.cc)
add_executable(test_talk_to_server test_talk_to_server.cc)
add_executable(regexp_tester regexp_tester.cc)
target_link_libraries(cadabra-server cadabra_server ${Boost_LIBRARIES} ${PYTHON_LIBRARIES} ${JSONCPP_LIB})
target_link_libraries(test_talk_to_server ${Boost_LIBRARIES} -lpthread)

install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/cadabra-server DESTINATION bin)


