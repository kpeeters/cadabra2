cmake_minimum_required(VERSION 2.8)

project(CadabraServer)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.9)
        message(FATAL_ERROR "GCC version must be at least 4.9 for regex support!")
    endif()
endif()

# Set path to additional cmake files
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/../cmake/modules")

# Locate Boost
find_package(Boost 1.45.0 COMPONENTS python system)
include_directories("${Boost_INCLUDE_DIRS}" "/usr/include/python2.7")
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(PkgConfig REQUIRED)
find_package(PythonLibs 2.7 REQUIRED)
find_package(websocketpp REQUIRED)            # from github
#pkg_check_modules(JSONCPP jsoncpp REQUIRED)   # libjsoncpp-dev
find_library(JSONCPP_LIB jsoncpp REQUIRED)
message("-- Found JSONCPP ${JSONCPP_LIB}")

if(CMAKE_COMPILER_IS_GNUCXX)
  add_definitions("-Wall -g -Wno-unused-but-set-variable")
endif()
add_definitions("-D_WEBSOCKETPP_CPP11_STL_")

# Create the server library
add_library(cadabra_server SHARED Server.cc)
target_link_libraries(cadabra_server ${Boost_LIBRARIES} ${PYTHON_LIBRARIES} ${JSONCPP_LIB})

# Create the client library. 
add_library(cadabra_client SHARED ComputeThread.cc DocumentThread.cc DataCell.cc Actions.cc)
target_link_libraries(cadabra_client ${Boost_LIBRARIES} ${JSONCPP_LIB})

# Create server binary
include_directories("." ${WEBSOCKETPP_INCLUDE_DIR} ${JSONCPP_INCLUDE_DIR})
add_executable(test_server test_server.cc)
add_executable(test_talk_to_server test_talk_to_server.cc)
target_link_libraries(test_server cadabra_server ${Boost_LIBRARIES} ${PYTHON_LIBRARIES} ${JSONCPP_LIB})
target_link_libraries(test_talk_to_server ${Boost_LIBRARIES} -lpthread)

# Create client test programs
#include_directories("." ${WEBSOCKETPP_INCLUDE_DIR})
#add_executable(test_client test_client.cc)
#target_link_libraries(test_client cadabra_client -lpthread)

