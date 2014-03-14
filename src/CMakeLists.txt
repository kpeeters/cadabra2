cmake_minimum_required(VERSION 2.8)

project(Cadabra)

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

if(CMAKE_COMPILER_IS_CLANG)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wno-c++11-extensions")
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
  add_definitions("-Wall -g -Wno-unused-but-set-variable")
endif()

set(LOCAL_SRC_FILES 
  Stopwatch.cc
  Exceptions.cc
  Algorithm.cc
  DisplayTeX.cc
  Cleanup.cc
  Combinatorics.cc
  YoungTab.cc
  Parser.cc
  PreProcessor.cc
  Storage.cc
  Compare.cc
  Kernel.cc
  Props.cc
  Exchange.cc
  PythonException.cc
  PythonCdb.cc
  PreClean.cc
  properties/Accent.cc
  properties/AntiCommuting.cc
  properties/AntiSymmetric.cc
  properties/CommutingAsProduct.cc
  properties/CommutingAsSum.cc
  properties/CommutingBehaviour.cc
  properties/Coordinate.cc
  properties/Derivative.cc
  properties/DiracBar.cc
  properties/Distributable.cc
  properties/GammaMatrix.cc
  properties/GammaTraceless.cc
  properties/ImplicitIndex.cc
  properties/Indices.cc
  properties/Integer.cc
  properties/LaTeXForm.cc
  properties/Matrix.cc
  properties/NumericalFlat.cc
  properties/PartialDerivative.cc
  properties/SortOrder.cc
  properties/Spinor.cc
  properties/Symbol.cc
  properties/Traceless.cc
  properties/Weight.cc
  properties/WeightInherit.cc
  modules/xperm_new.cc
  algorithms/collect_terms.cc
  algorithms/distribute.cc
  algorithms/rename_dummies.cc
  algorithms/flatten_product.cc
  algorithms/prodcollectnum.cc
  algorithms/sort_product.cc
  algorithms/substitute.cc
)
set(PUBLIC_HEADER_FILES
  Parser.hh
  Storage.hh
)

# Create Python dynamic library 'cadabra' without lib prefix.
include_directories(".")
add_library(pcadabra SHARED ${LOCAL_SRC_FILES})
set_target_properties(pcadabra PROPERTIES PREFIX "" SUFFIX ".so" IMPORT_SUFFIX ".so")
set_target_properties(pcadabra PROPERTIES PUBLIC_HEADER "${PUBLIC_HEADER_FILES}")
target_link_libraries(pcadabra ${PCRE_LIBRARIES} gmp gmpxx ${Boost_LIBRARIES} python)

add_library(example SHARED ptest.cc)
set_target_properties(example PROPERTIES PREFIX "" SUFFIX ".so")
target_link_libraries(example ${Boost_LIBRARIES} python)
