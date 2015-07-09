cmake_minimum_required(VERSION 2.8)

project(Cadabra)


# Set path to additional cmake files
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/../cmake/modules")

# Speedup compilation using cotire.
# include(cotire)

# Locate Boost and Boost.Python.
find_package(Boost 1.45.0)
include_directories("${Boost_INCLUDE_DIRS}" "/usr/include/python2.7")
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost 1.45.0 COMPONENTS python)
find_program(PYTHON "python" REQUIRED)

# Locate gmpxx
find_library(GMP_LIB gmp REQUIRED)
find_library(GMPXX_LIB gmpxx REQUIRED)
FIND_PACKAGE(PythonLibs REQUIRED)
message("-- Found gmp ${GMP_LIB}")
message("-- Found gmpxx ${GMPXX_LIB}")
message("-- Found python ${PYTHON_LIBRARIES}")

# Locate pcre
find_package(PCRE REQUIRED)

if(CMAKE_COMPILER_IS_GNUCXX)
  add_definitions("-Wall -g -Wno-unused-but-set-variable")
endif()

set(LOCAL_SRC_FILES 
  Algorithm.cc
  Cleanup.cc
  Combinatorics.cc
  Compare.cc
  DisplayTeX.cc
  Exceptions.cc
  Exchange.cc
  IndexIterator.cc
  Kernel.cc
  Parser.cc
  PreClean.cc
  PreProcessor.cc
  Props.cc
  PythonCdb.cc
  PythonException.cc
  Stopwatch.cc
  Storage.cc
  YoungTab.cc
  properties/Accent.cc
  properties/AntiCommuting.cc
  properties/AntiSymmetric.cc
  properties/Commuting.cc
  properties/CommutingAsProduct.cc
  properties/CommutingAsSum.cc
  properties/CommutingBehaviour.cc
  properties/Coordinate.cc
  properties/DAntiSymmetric.cc
  properties/Depends.cc
  properties/Derivative.cc
  properties/Diagonal.cc
  properties/DiracBar.cc
  properties/Distributable.cc
  properties/FilledTableau.cc
  properties/GammaMatrix.cc
  properties/GammaTraceless.cc
  properties/ImplicitIndex.cc
  properties/Indices.cc
  properties/Integer.cc
  properties/KroneckerDelta.cc
  properties/LaTeXForm.cc
  properties/Matrix.cc
  properties/Metric.cc
  properties/NonCommuting.cc
  properties/NumericalFlat.cc
  properties/PartialDerivative.cc
  properties/RiemannTensor.cc
  properties/SatisfiesBianchi.cc
  properties/SelfAntiCommuting.cc
  properties/SelfCommuting.cc
  properties/SortOrder.cc
  properties/Spinor.cc
  properties/Symbol.cc
  properties/Symmetric.cc
  properties/Tableau.cc
  properties/TableauBase.cc
  properties/TableauSymmetry.cc
  properties/Traceless.cc
  properties/Weight.cc
  properties/WeightInherit.cc
  properties/WeylTensor.cc
  modules/xperm_new.cc
  algorithms/canonicalise.cc
  algorithms/collect_terms.cc
  algorithms/decompose_product.cc
  algorithms/distribute.cc
#  algorithms/drop_weight.cc
  algorithms/eliminate_kronecker.cc
  algorithms/flatten_product.cc
  algorithms/flatten_sum.cc
  algorithms/indexsort.cc
  algorithms/join_gamma.cc
  algorithms/keep_terms.cc
  algorithms/lr_tensor.cc
  algorithms/order.cc
  algorithms/prodcollectnum.cc
  algorithms/product_rule.cc
#  algorithms/reduce_sub.cc
  algorithms/rename_dummies.cc
  algorithms/sort_product.cc
  algorithms/split_index.cc
  algorithms/substitute.cc
  algorithms/sym.cc
  algorithms/tab_basics.cc
  algorithms/unwrap.cc
  algorithms/young_project.cc
  algorithms/young_project_product.cc
  algorithms/young_project_tensor.cc
)
set(PUBLIC_HEADER_FILES
  Parser.hh
  Storage.hh
)
set(IMAGES
  ../images/cadabra.png
)

# Python installation logic.
#set(SETUP_PY_IN "${CMAKE_CURRENT_SOURCE_DIR}/setup.py.in")
#set(SETUP_PY    "${CMAKE_CURRENT_BINARY_DIR}/setup.py")
#configure_file(${SETUP_PY_IN} ${SETUP_PY})
#install(CODE "execute_process(COMMAND ${PYTHON} ${SETUP_PY} install)")

# import site; print site.getsitepackages()[0]

execute_process(COMMAND ${PYTHON} -c "import site; print site.getsitepackages()[0];" 
  OUTPUT_VARIABLE python_site_path OUTPUT_STRIP_TRAILING_WHITESPACE)

message("-- Python site path at ${python_site_path}")
install(FILES
  "${PROJECT_BINARY_DIR}/cadabra2.so"
  "${PROJECT_SOURCE_DIR}/cadabra2_defaults.py" DESTINATION "${python_site_path}")

install(PROGRAMS
  "${PROJECT_SOURCE_DIR}/cadabra2" DESTINATION bin)

# FIXME: sort out how to install auxiliary files in a system-independent way
install(FILES
  "${PROJECT_SOURCE_DIR}/../images/cadabra.png" DESTINATION share/cadabra2/images)


# Create Python dynamic library 'cadabra' without lib prefix.
include_directories(".")
add_library(cadabra2 SHARED ${LOCAL_SRC_FILES})
add_library(scopetest SHARED scopetest.cc)
set_target_properties(cadabra2 PROPERTIES PREFIX "" SUFFIX ".so" IMPORT_SUFFIX ".so")
set_target_properties(cadabra2 PROPERTIES PUBLIC_HEADER "${PUBLIC_HEADER_FILES}")
set_target_properties(scopetest PROPERTIES PREFIX "" SUFFIX ".so" IMPORT_SUFFIX ".so")

# Activate cotire for the entire library. This takes _a lot_ of disk space.
# cotire(cadabra2)

#if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clangx")
#  target_link_libraries(cadabra2 ${PCRE_LIBRARIES} ${GMP_LIB} ${Boost_LIBRARIES} python)
#else()

target_link_libraries(cadabra2 ${PCRE_LIBRARIES} ${GMPXX_LIB} ${GMP_LIB} ${Boost_LIBRARIES} ${PYTHON_LIBRARIES} )

target_link_libraries(scopetest ${Boost_LIBRARIES} ${PYTHON_LIBRARIES})

#endif()

