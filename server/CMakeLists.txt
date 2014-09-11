cmake_minimum_required(VERSION 2.8)

project(CadabraServer)

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

# Set path to additional cmake files
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/../cmake/modules")

# Locate Boost
find_package(Boost 1.45.0 COMPONENTS python system)
include_directories("${Boost_INCLUDE_DIRS}" "/usr/include/python2.7" "../external/json_spirit/")
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(PkgConfig REQUIRED)
find_package(PythonLibs 2.7 REQUIRED)
find_package(websocketpp REQUIRED)            # from github
pkg_check_modules(JSONCPP jsoncpp REQUIRED)   # libjsoncpp-dev

if(CMAKE_COMPILER_IS_GNUCXX)
  add_definitions("-Wall -g -Wno-unused-but-set-variable -D_WEBSOCKETPP_CPP11_STL_")
endif()

# Create the server library
add_library(cadabra_server SHARED Server.cc)
target_link_libraries(cadabra_server ${Boost_LIBRARIES} ${PYTHON_LIBRARIES})

# Create the client library
add_library(cadabra_client SHARED Client.cc)
target_link_libraries(cadabra_client ${Boost_LIBRARIES})

# Create server binary
include_directories("." ${WEBSOCKETPP_INCLUDE_DIR} ${JSONCPP_INCLUDE_DIR})
add_executable(test_server test_server.cc)
add_executable(test_talk_to_server test_talk_to_server.cc)
target_link_libraries(test_server cadabra_server ${Boost_LIBRARIES} ${PYTHON_LIBRARIES} -ljsoncpp)
target_link_libraries(test_talk_to_server ${Boost_LIBRARIES} -lpthread)

# Create client test programs
include_directories("." ${WEBSOCKETPP_INCLUDE_DIR} ${JSONCPP_INCLUDE_DIR})
add_executable(test_client test_client.cc)
