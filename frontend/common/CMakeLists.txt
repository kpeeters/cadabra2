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

find_package(Threads)
find_package(Boost 1.45.0 COMPONENTS system)
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)

include_directories(
  ../../core
  ../../libs/internal/include
  ../../libs/whereami
  "${Boost_INCLUDE_DIRS}"
  )

if(CMAKE_COMPILER_IS_GNUCXX)
  add_definitions("-Wall -g -Wno-unused-but-set-variable")
endif()

# Create library; disabled to make life easier on Windows
include_directories("." "../../libs/internal/include")
# add_library(texengine SHARED TeXEngine.cc lodepng.cc exec-stream.cc)
# target_link_libraries(texengine ${Boost_LIBRARIES} Threads::Threads)

add_executable(test_tex test_tex.cc TeXEngine.cc lodepng.cc exec-stream.cc ../../core/InstallPrefix.cc ../../libs/whereami/whereami.c)
target_link_libraries(test_tex ${Boost_LIBRARIES} Threads::Threads)
# install(TARGETS texengine ARCHIVE DESTINATION lib LIBRARY DESTINATION lib)

