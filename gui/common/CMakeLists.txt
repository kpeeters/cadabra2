cmake_minimum_required(VERSION 2.8)

project(TeXEngine)

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

# Set path to additional cmake files
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/../../cmake/modules")

# Locate Boost
find_package(Boost 1.45.0 COMPONENTS filesystem)
include_directories("${Boost_INCLUDE_DIRS}")
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)

if(CMAKE_COMPILER_IS_GNUCXX)
  add_definitions("-Wall -g -Wno-unused-but-set-variable")
endif()

# Create library
include_directories(".")
add_library(texengine SHARED TeXEngine.cc lodepng.cc exec-stream.cc)
target_link_libraries(texengine ${Boost_LIBRARIES} pthread)
add_executable(test test.cc)
target_link_libraries(test texengine)
