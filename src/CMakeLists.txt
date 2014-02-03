cmake_minimum_required(VERSION 2.8)

project(cadabra)

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

# Locate Boost and Boost.Python.
find_package(Boost 1.45.0)
include_directories("${Boost_INCLUDE_DIRS}" "/usr/include/python2.7")
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost 1.45.0 COMPONENTS python)

# Create python dynamic library 'cadabra' without lib prefix.
add_library(cadabra SHARED pythonify.cc)
set_target_properties(cadabra PROPERTIES PREFIX "")
target_link_libraries(cadabra ${Boost_LIBRARIES})

if(CMAKE_COMPILER_IS_GNUCXX)
  add_definitions("-Wall")
endif()

