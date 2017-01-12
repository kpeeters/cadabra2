
cmake_minimum_required(VERSION 2.8)

project(Cadabra)


# Set path to additional cmake files
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/../cmake/modules")

message("-- CMake version = ${CMAKE_VERSION}")

if (POLICY CMP0042)
  cmake_policy(SET CMP0042 NEW)
endif(POLICY CMP0042)


# Locate gmpxx. On Homebrew there seems to be constantly something wrong with
# the pkgconfig for gmpxx. So we just add the include path by hand.
if(APPLE)
   add_definitions("-I/usr/local/include -I/opt/local/include")
endif()
find_library(GMP_LIB gmp REQUIRED)
find_library(GMPXX_LIB gmpxx REQUIRED)
message("-- Found gmp ${GMP_LIB}")
message("-- Found gmpxx ${GMPXX_LIB}")

# Locate pcre
find_package(PCRE REQUIRED)

add_definitions("-DNO_SYMPY")
if(CMAKE_COMPILER_IS_GNUCXX)
  add_definitions("-Wall -g -Wno-unused-but-set-variable")
endif()
if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O2")
endif()

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O2")
endif()

set(LOCAL_SRC_FILES
  ${CMAKE_SOURCE_DIR}/../core/Algorithm.cc
  ${CMAKE_SOURCE_DIR}/../core/Cleanup.cc
  ${CMAKE_SOURCE_DIR}/../core/Combinatorics.cc
  ${CMAKE_SOURCE_DIR}/../core/Compare.cc
  ${CMAKE_SOURCE_DIR}/../core/DisplayBase.cc
  ${CMAKE_SOURCE_DIR}/../core/DisplayTeX.cc
  ${CMAKE_SOURCE_DIR}/../core/DisplaySympy.cc
  ${CMAKE_SOURCE_DIR}/../core/DisplayTerminal.cc
  ${CMAKE_SOURCE_DIR}/../core/TerminalStream.cc
  ${CMAKE_SOURCE_DIR}/../core/Exceptions.cc
  ${CMAKE_SOURCE_DIR}/../core/Exchange.cc
  ${CMAKE_SOURCE_DIR}/../core/Functional.cc
  ${CMAKE_SOURCE_DIR}/../core/IndexIterator.cc
  ${CMAKE_SOURCE_DIR}/../core/Kernel.cc
  ${CMAKE_SOURCE_DIR}/../core/Parser.cc
  ${CMAKE_SOURCE_DIR}/../core/PreClean.cc
  ${CMAKE_SOURCE_DIR}/../core/PreProcessor.cc
  ${CMAKE_SOURCE_DIR}/../core/Props.cc
  ${CMAKE_SOURCE_DIR}/../core/Stopwatch.cc
  ${CMAKE_SOURCE_DIR}/../core/Storage.cc
  ${CMAKE_SOURCE_DIR}/../core/Symbols.cc
  ${CMAKE_SOURCE_DIR}/../core/YoungTab.cc
  ${CMAKE_SOURCE_DIR}/../core/properties/Accent.cc
  ${CMAKE_SOURCE_DIR}/../core/properties/AntiCommuting.cc
  ${CMAKE_SOURCE_DIR}/../core/properties/AntiSymmetric.cc
  ${CMAKE_SOURCE_DIR}/../core/properties/Commuting.cc
  ${CMAKE_SOURCE_DIR}/../core/properties/CommutingAsProduct.cc
  ${CMAKE_SOURCE_DIR}/../core/properties/CommutingAsSum.cc
  ${CMAKE_SOURCE_DIR}/../core/properties/CommutingBehaviour.cc
  ${CMAKE_SOURCE_DIR}/../core/properties/Coordinate.cc
  ${CMAKE_SOURCE_DIR}/../core/properties/DAntiSymmetric.cc
  ${CMAKE_SOURCE_DIR}/../core/properties/Depends.cc
  ${CMAKE_SOURCE_DIR}/../core/properties/DependsInherit.cc
  ${CMAKE_SOURCE_DIR}/../core/properties/Derivative.cc
  ${CMAKE_SOURCE_DIR}/../core/properties/Diagonal.cc
  ${CMAKE_SOURCE_DIR}/../core/properties/DifferentialForm.cc
  ${CMAKE_SOURCE_DIR}/../core/properties/DiracBar.cc
  ${CMAKE_SOURCE_DIR}/../core/properties/Distributable.cc
  ${CMAKE_SOURCE_DIR}/../core/properties/EpsilonTensor.cc
  ${CMAKE_SOURCE_DIR}/../core/properties/ExteriorDerivative.cc
  ${CMAKE_SOURCE_DIR}/../core/properties/FilledTableau.cc
  ${CMAKE_SOURCE_DIR}/../core/properties/GammaMatrix.cc
  ${CMAKE_SOURCE_DIR}/../core/properties/GammaTraceless.cc
  ${CMAKE_SOURCE_DIR}/../core/properties/ImplicitIndex.cc
  ${CMAKE_SOURCE_DIR}/../core/properties/Indices.cc
  ${CMAKE_SOURCE_DIR}/../core/properties/Integer.cc
  ${CMAKE_SOURCE_DIR}/../core/properties/InverseMetric.cc
  ${CMAKE_SOURCE_DIR}/../core/properties/KroneckerDelta.cc
  ${CMAKE_SOURCE_DIR}/../core/properties/LaTeXForm.cc
  ${CMAKE_SOURCE_DIR}/../core/properties/Matrix.cc
  ${CMAKE_SOURCE_DIR}/../core/properties/Metric.cc
  ${CMAKE_SOURCE_DIR}/../core/properties/NonCommuting.cc
  ${CMAKE_SOURCE_DIR}/../core/properties/NumericalFlat.cc
  ${CMAKE_SOURCE_DIR}/../core/properties/PartialDerivative.cc
  ${CMAKE_SOURCE_DIR}/../core/properties/RiemannTensor.cc
  ${CMAKE_SOURCE_DIR}/../core/properties/SatisfiesBianchi.cc
  ${CMAKE_SOURCE_DIR}/../core/properties/SelfAntiCommuting.cc
  ${CMAKE_SOURCE_DIR}/../core/properties/SelfCommuting.cc
  ${CMAKE_SOURCE_DIR}/../core/properties/SelfNonCommuting.cc
  ${CMAKE_SOURCE_DIR}/../core/properties/SortOrder.cc
  ${CMAKE_SOURCE_DIR}/../core/properties/Spinor.cc
  ${CMAKE_SOURCE_DIR}/../core/properties/Symbol.cc
  ${CMAKE_SOURCE_DIR}/../core/properties/Symmetric.cc
  ${CMAKE_SOURCE_DIR}/../core/properties/Tableau.cc
  ${CMAKE_SOURCE_DIR}/../core/properties/TableauBase.cc
  ${CMAKE_SOURCE_DIR}/../core/properties/TableauSymmetry.cc
  ${CMAKE_SOURCE_DIR}/../core/properties/Traceless.cc
  ${CMAKE_SOURCE_DIR}/../core/properties/Weight.cc
  ${CMAKE_SOURCE_DIR}/../core/properties/WeightInherit.cc
  ${CMAKE_SOURCE_DIR}/../core/properties/WeylTensor.cc
  ${CMAKE_SOURCE_DIR}/../core/modules/xperm_new.cc
  ${CMAKE_SOURCE_DIR}/../core/algorithms/canonicalise.cc
  ${CMAKE_SOURCE_DIR}/../core/algorithms/collect_components.cc
  ${CMAKE_SOURCE_DIR}/../core/algorithms/collect_factors.cc
  ${CMAKE_SOURCE_DIR}/../core/algorithms/collect_terms.cc
  ${CMAKE_SOURCE_DIR}/../core/algorithms/combine.cc
  ${CMAKE_SOURCE_DIR}/../core/algorithms/complete.cc
  ${CMAKE_SOURCE_DIR}/../core/algorithms/decompose_product.cc
  ${CMAKE_SOURCE_DIR}/../core/algorithms/distribute.cc
  ${CMAKE_SOURCE_DIR}/../core/algorithms/drop_weight.cc
  ${CMAKE_SOURCE_DIR}/../core/algorithms/einsteinify.cc
  ${CMAKE_SOURCE_DIR}/../core/algorithms/eliminate_kronecker.cc
  ${CMAKE_SOURCE_DIR}/../core/algorithms/eliminate_metric.cc
  ${CMAKE_SOURCE_DIR}/../core/algorithms/epsilon_to_delta.cc
  ${CMAKE_SOURCE_DIR}/../core/algorithms/evaluate.cc
  ${CMAKE_SOURCE_DIR}/../core/algorithms/expand.cc
  ${CMAKE_SOURCE_DIR}/../core/algorithms/expand_delta.cc
  ${CMAKE_SOURCE_DIR}/../core/algorithms/expand_diracbar.cc
  ${CMAKE_SOURCE_DIR}/../core/algorithms/expand_power.cc
  ${CMAKE_SOURCE_DIR}/../core/algorithms/factor_in.cc
  ${CMAKE_SOURCE_DIR}/../core/algorithms/factor_out.cc
  ${CMAKE_SOURCE_DIR}/../core/algorithms/fierz.cc
  ${CMAKE_SOURCE_DIR}/../core/algorithms/flatten_product.cc
  ${CMAKE_SOURCE_DIR}/../core/algorithms/flatten_sum.cc
  ${CMAKE_SOURCE_DIR}/../core/algorithms/indexsort.cc
  ${CMAKE_SOURCE_DIR}/../core/algorithms/integrate_by_parts.cc
  ${CMAKE_SOURCE_DIR}/../core/algorithms/join_gamma.cc
  ${CMAKE_SOURCE_DIR}/../core/algorithms/keep_terms.cc
  ${CMAKE_SOURCE_DIR}/../core/algorithms/lr_tensor.cc
  ${CMAKE_SOURCE_DIR}/../core/algorithms/order.cc
  ${CMAKE_SOURCE_DIR}/../core/algorithms/product_rule.cc
  ${CMAKE_SOURCE_DIR}/../core/algorithms/reduce_delta.cc
  ${CMAKE_SOURCE_DIR}/../core/algorithms/rename_dummies.cc
  ${CMAKE_SOURCE_DIR}/../core/algorithms/rewrite_indices.cc
  ${CMAKE_SOURCE_DIR}/../core/algorithms/sort_product.cc
  ${CMAKE_SOURCE_DIR}/../core/algorithms/sort_sum.cc
  ${CMAKE_SOURCE_DIR}/../core/algorithms/split_gamma.cc
  ${CMAKE_SOURCE_DIR}/../core/algorithms/split_index.cc
  ${CMAKE_SOURCE_DIR}/../core/algorithms/substitute.cc
  ${CMAKE_SOURCE_DIR}/../core/algorithms/sym.cc
  ${CMAKE_SOURCE_DIR}/../core/algorithms/tab_basics.cc
  ${CMAKE_SOURCE_DIR}/../core/algorithms/take_match.cc
  ${CMAKE_SOURCE_DIR}/../core/algorithms/replace_match.cc
  ${CMAKE_SOURCE_DIR}/../core/algorithms/unwrap.cc
  ${CMAKE_SOURCE_DIR}/../core/algorithms/vary.cc
  ${CMAKE_SOURCE_DIR}/../core/algorithms/young_project.cc
  ${CMAKE_SOURCE_DIR}/../core/algorithms/young_project_product.cc
  ${CMAKE_SOURCE_DIR}/../core/algorithms/young_project_tensor.cc
  SympyDummy.cc
)
set(MAIN_HEADERS
  ${CMAKE_SOURCE_DIR}/../core/Algorithm.hh
  ${CMAKE_SOURCE_DIR}/../core/DisplayTerminal.hh
  ${CMAKE_SOURCE_DIR}/../core/Parser.hh
  ${CMAKE_SOURCE_DIR}/../core/Storage.hh
  ${CMAKE_SOURCE_DIR}/../core/tree.hh
  )
FILE(GLOB ALGO_HEADERS ${CMAKE_SOURCE_DIR}/../core/algorithms/*.hh)
FILE(GLOB PROP_HEADERS ${CMAKE_SOURCE_DIR}/../core/properties/*.hh)

# Create the 'variable-substituted' cadabra2 inside the source tree so that we can
# run the tests and pick up cadabra2_defaults.py from there.

include_directories("${CMAKE_SOURCE_DIR}/../core" ".")

# Scalar backend options.
option(USE_TREETRACKER "Use the TreeTracker scalar backend" ON)
set(PATH_TREETRACKER   "./treetracker" CACHE STRING "Path to the TreeTracker library")
configure_file("${PROJECT_SOURCE_DIR}/../core/ScalarConfig.hh.in" "${PROJECT_BINARY_DIR}/c++lib/ScalarConfig.hh")
if(USE_TREETRACKER)
  if(EXISTS "${PATH_TREETRACKER}/treetracker.h" OR EXISTS "${CMAKE_SOURCE_DIR}/${PATH_TREETRACKER}/treetracker.h")
	 include_directories(${PATH_TREETRACKER})
	 message("-- Using the TreeTracker scalar backend at ${PATH_TREETRACKER}")
  else()
	 message(FATAL_ERROR "-- Cannot find the 'treetracker.h' file in '${PATH_TREETRACKER}'")
  endif()
else()
  message("-- Not using the TreeTracker scalar backend")
endif()
# .. more to come

add_library(cadabra2++ SHARED ${LOCAL_SRC_FILES})
set_target_properties(cadabra2++ PROPERTIES PUBLIC_HEADER "${PUBLIC_HEADER_FILES}")
target_link_libraries(cadabra2++ ${PCRE_LIBRARIES} ${GMPXX_LIB} ${GMP_LIB})

set(INSTALL_LIB_DIR     lib                CACHE PATH "Installation directory for library")
set(INSTALL_INCLUDE_DIR include/cadabra2++ CACHE PATH "Installation directory for header files")

install(TARGETS  cadabra2++       LIBRARY DESTINATION "${INSTALL_LIB_DIR}")
install(FILES    ${MAIN_HEADERS}  DESTINATION "${INSTALL_INCLUDE_DIR}")
install(FILES    ${ALGO_HEADERS}  DESTINATION "${INSTALL_INCLUDE_DIR}/algorithms")
install(FILES    ${PROP_HEADERS}  DESTINATION "${INSTALL_INCLUDE_DIR}/properties")

install(DIRECTORY DESTINATION bin DIRECTORY_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
  GROUP_READ GROUP_EXECUTE
  WORLD_READ WORLD_EXECUTE)

install(DIRECTORY DESTINATION lib DIRECTORY_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
  GROUP_READ GROUP_EXECUTE
  WORLD_READ WORLD_EXECUTE)



add_executable(simple simple.cc)
target_link_libraries(simple cadabra2++)

