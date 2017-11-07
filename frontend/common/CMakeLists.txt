cmake_minimum_required(VERSION 2.8)

project(TeXEngine)

if (POLICY CMP0042)
  cmake_policy(SET CMP0042 NEW)
endif(POLICY CMP0042)

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

# Set path to additional cmake files
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/../../cmake/modules")

# Locate Boost
add_definitions("-DBOOST_ALL_NO_LIB")
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost 1.45.0 COMPONENTS system filesystem)
include_directories("${Boost_INCLUDE_DIRS}")

if(CMAKE_COMPILER_IS_GNUCXX)
  add_definitions("-Wall -g -Wno-unused-but-set-variable")
endif()

# Create library
include_directories(".")
add_library(texengine SHARED TeXEngine.cc lodepng.cc exec-stream.cc)
target_link_libraries(texengine ${Boost_LIBRARIES})
if(NOT WIN32)
	target_link_libraries(texengine pthread)
endif()
add_executable(test_tex test_tex.cc TeXEngine.cc lodepng.cc exec-stream.cc)
if(WIN32)
	target_link_libraries(test_tex ${Boost_LIBRARIES})
else()
	target_link_libraries(test_tex texengine)
endif()
install(TARGETS texengine ARCHIVE DESTINATION lib LIBRARY DESTINATION lib)

