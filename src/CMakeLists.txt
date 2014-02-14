cmake_minimum_required(VERSION 2.8)

project(Cadabra)

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

# Set path to additional cmake files
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/../cmake/modules")

# Locate Boost and Boost.Python.
find_package(Boost 1.45.0)
include_directories("${Boost_INCLUDE_DIRS}" "/usr/include/python2.7")
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost 1.45.0 COMPONENTS python)

# Locate gmpxx
find_package(GMPXX REQUIRED)

# Locate pcre
find_package(PCRE REQUIRED)

if(CMAKE_COMPILER_IS_GNUCXX)
  add_definitions("-Wall")
endif()

set(LOCAL_SRC_FILES 
  Stopwatch.cc
  Exceptions.cc
  Algorithm.cc
  Cleanup.cc
  Combinatorics.cc
  YoungTab.cc
  Parser.cc
  PreProcessor.cc
  Storage.cc
  Props.cc
  CoreProps.cc
  Numerical.cc
  Exchange.cc
  PythonCdb.cc
  modules/Algebra.cc
  properties/Weight.cc
  properties/WeightInherit.cc
  properties/Spinor.cc
)
set(PUBLIC_HEADER_FILES
  Parser.hh
  Storage.hh
)

# Create Python dynamic library 'cadabra' without lib prefix.
include_directories(".")
add_library(cadabra SHARED ${LOCAL_SRC_FILES})
set_target_properties(cadabra PROPERTIES PREFIX "")
set_target_properties(cadabra PROPERTIES PUBLIC_HEADER "${PUBLIC_HEADER_FILES}")
target_link_libraries(cadabra  ${PCRE_LIBRARIES} gmp gmpxx ${Boost_LIBRARIES})
