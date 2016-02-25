cmake_minimum_required(VERSION 2.8)

project(Cadabra)


# Set path to additional cmake files
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/../cmake/modules")

if (POLICY CMP0042)
  cmake_policy(SET CMP0042 NEW)
endif(POLICY CMP0042)

# Speedup compilation using cotire.
# include(cotire)

# Locate Boost and Boost.Python.
find_package(Boost 1.45.0)
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost 1.45.0 COMPONENTS python)
set(Python_ADDITIONAL_VERSIONS 2.7)
find_package(PythonInterp REQUIRED)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 3.5)
        message(FATAL_ERROR "Clang version must be at least 3.5 to avoid known bugs.")
    endif()
	 add_definitions("-Wno-potentially-evaluated-expression")
endif()

# Locate gmpxx
find_library(GMP_LIB gmp REQUIRED)
find_library(GMPXX_LIB gmpxx REQUIRED)
FIND_PACKAGE(PythonLibs REQUIRED)
message("-- Found gmp ${GMP_LIB}")
message("-- Found gmpxx ${GMPXX_LIB}")
message("-- Found python ${PYTHON_LIBRARIES}")
include_directories("${Boost_INCLUDE_DIRS}" ${PYTHON_INCLUDE_DIRS})
message("-- Python include dirs: ${PYTHON_INCLUDE_DIRS}")

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
  DisplayBase.cc
  DisplayTeX.cc
  DisplaySympy.cc
  DisplayTerminal.cc
  Exceptions.cc
  Exchange.cc
  Functional.cc
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
  SympyCdb.cc
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
  properties/InverseMetric.cc
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
  algorithms/collect_factors.cc
  algorithms/collect_terms.cc
  algorithms/complete.cc
  algorithms/decompose_product.cc
  algorithms/distribute.cc
#  algorithms/drop_weight.cc
  algorithms/eliminate_kronecker.cc
  algorithms/evaluate.cc
  algorithms/expand_diracbar.cc
  algorithms/flatten_product.cc
  algorithms/flatten_sum.cc
  algorithms/indexsort.cc
  algorithms/join_gamma.cc
  algorithms/keep_terms.cc
  algorithms/lr_tensor.cc
  algorithms/order.cc
  algorithms/product_rule.cc
#  algorithms/reduce_sub.cc
  algorithms/rename_dummies.cc
  algorithms/sort_product.cc
  algorithms/split_index.cc
  algorithms/substitute.cc
  algorithms/sym.cc
  algorithms/tab_basics.cc
  algorithms/unwrap.cc
  algorithms/vary.cc
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
#install(CODE "execute_process(COMMAND ${PYTHON_EXECUTABLE} ${SETUP_PY} install)")

# import site; print site.getsitepackages()[0]

execute_process(COMMAND ${PYTHON_EXECUTABLE} -c "import site; print site.getsitepackages()[0];"
  OUTPUT_VARIABLE python_site_path OUTPUT_STRIP_TRAILING_WHITESPACE)

message("-- Python site path at ${python_site_path}")
install(FILES
  "${PROJECT_BINARY_DIR}/cadabra2.so"
  "${PROJECT_SOURCE_DIR}/cadabra2_defaults.py" DESTINATION "${python_site_path}")

install(PROGRAMS
  "${PROJECT_SOURCE_DIR}/cadabra2" DESTINATION bin)


# Create Python dynamic library 'cadabra' without lib prefix.
include_directories(".")
add_library(cadabra2 SHARED ${LOCAL_SRC_FILES})
set_target_properties(cadabra2 PROPERTIES PREFIX "" SUFFIX ".so" IMPORT_SUFFIX ".so")
set_target_properties(cadabra2 PROPERTIES PUBLIC_HEADER "${PUBLIC_HEADER_FILES}")

# Activate cotire for the entire library. This takes _a lot_ of disk space.
# cotire(cadabra2)

#if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clangx")
#  target_link_libraries(cadabra2 ${PCRE_LIBRARIES} ${GMP_LIB} ${Boost_LIBRARIES} python)
#else()

target_link_libraries(cadabra2 ${PCRE_LIBRARIES} ${GMPXX_LIB} ${GMP_LIB} ${Boost_LIBRARIES} ${PYTHON_LIBRARIES} )

#endif()

