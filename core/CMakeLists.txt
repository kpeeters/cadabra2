
cmake_minimum_required(VERSION 3.1)
project(Cadabra)
message("------------------------------------------------------------------------\nConfiguring 'core'")

# Set path to additional cmake files
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/../cmake/modules")

# Locate Mathematica (optional)
set(Mathematica_USE_STATIC_LIBRARIES TRUE)
#find_package(Mathematica COMPONENTS WSTP)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 3.5)
        message(FATAL_ERROR "Clang version must be at least 3.5 to avoid known bugs.")
    endif()
#	 add_definitions("-Wno-potentially-evaluated-expression")
endif()



# Locate gmpxx. On Homebrew there seems to be constantly something wrong with
# the pkgconfig for gmpxx. So we just add the include path by hand.
if(APPLE)
   add_definitions("-I/usr/local/include -I/opt/local/include")
endif()
find_library(GMP_LIB gmp REQUIRED)
find_library(GMPXX_LIB gmpxx REQUIRED)
message("-- Found gmp ${GMP_LIB}")
message("-- Found gmpxx ${GMPXX_LIB}")
message("-- Found python ${PYTHON_LIBRARIES}")
include_directories(${PYTHON_INCLUDE_DIRS})

# Locate Boost
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost 1.53.0 COMPONENTS regex)

message("-- Python include dirs: ${PYTHON_INCLUDE_DIRS}")
message("-- Boost include dirs: ${Boost_INCLUDE_DIRS}")

# Mathematica WSTP test
if(MATHEMATICA_FOUND)
  message("-- Building with Mathematica support (linking against ${Mathematica_VERSION})")
  include_directories(${Mathematica_WSTP_INCLUDE_DIR})
  add_executable(test_wstp         test_wstp.cc)
  target_link_libraries(test_wstp ${Mathematica_WSTP_LIBRARIES})
else()
  message("-- Building without Mathematica support")
endif()

# Locate pcre
find_package(PCRE REQUIRED)

if(CMAKE_COMPILER_IS_GNUCXX)
  add_definitions("-Wall -g -Wno-unused-but-set-variable")
endif()

set(LOCAL_SRC_FILES
  PythonCdb.cc
  ProgressMonitor.cc
  Bridge.cc
  Algorithm.cc
  Cleanup.cc
  Combinatorics.cc
  Compare.cc
  DisplayBase.cc
  DisplayMMA.cc  
  DisplayTeX.cc
  DisplaySympy.cc
  DisplayTerminal.cc
  TerminalStream.cc
  Exceptions.cc
  Exchange.cc
  Functional.cc
  IndexIterator.cc
  Kernel.cc
  Parser.cc
  PreClean.cc
  PreProcessor.cc
  Props.cc
  PythonException.cc
  Stopwatch.cc
  Storage.cc
  Symbols.cc
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
  properties/DependsInherit.cc
  properties/Derivative.cc
  properties/Diagonal.cc
  properties/DifferentialForm.cc
  properties/DiracBar.cc
  properties/Distributable.cc
  properties/EpsilonTensor.cc
  properties/ExteriorDerivative.cc
  properties/FilledTableau.cc
  properties/GammaMatrix.cc
  properties/GammaTraceless.cc
  properties/ImaginaryI.cc
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
  properties/SelfNonCommuting.cc
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
  algorithms/collect_components.cc
  algorithms/collect_factors.cc
  algorithms/collect_terms.cc
  algorithms/combine.cc
  algorithms/complete.cc
  algorithms/decompose_product.cc
  algorithms/distribute.cc
  algorithms/drop_weight.cc
  algorithms/einsteinify.cc
  algorithms/eliminate_kronecker.cc
  algorithms/eliminate_metric.cc
  algorithms/epsilon_to_delta.cc
  algorithms/evaluate.cc
  algorithms/expand.cc
  algorithms/expand_delta.cc
  algorithms/expand_diracbar.cc
  algorithms/expand_power.cc
  algorithms/factor_in.cc
  algorithms/factor_out.cc
  algorithms/fierz.cc
  algorithms/flatten_product.cc
  algorithms/flatten_sum.cc
  algorithms/indexsort.cc
  algorithms/integrate_by_parts.cc
  algorithms/join_gamma.cc
  algorithms/keep_terms.cc
  algorithms/lr_tensor.cc
  algorithms/map_sympy.cc
  algorithms/order.cc
  algorithms/product_rule.cc
  algorithms/reduce_delta.cc
  algorithms/rename_dummies.cc
  algorithms/rewrite_indices.cc
  algorithms/sort_product.cc
  algorithms/sort_spinors.cc
  algorithms/sort_sum.cc
  algorithms/split_gamma.cc
  algorithms/split_index.cc
  algorithms/substitute.cc
  algorithms/sym.cc
  algorithms/tab_basics.cc
  algorithms/take_match.cc
  algorithms/replace_match.cc
  algorithms/unwrap.cc
  algorithms/vary.cc
  algorithms/young_project.cc
  algorithms/young_project_product.cc
  algorithms/young_project_tensor.cc
  )
if(MATHEMATICA_FOUND)
  set(LOCAL_SRC_FILES ${LOCAL_SRC_FILES}
	 MMACdb.cc  
	 algorithms/map_mma.cc
	 )
endif()
set(PUBLIC_HEADER_FILES
  Parser.hh
  Storage.hh
)
set(IMAGES
  ../images/cadabra.png
)

if("${PYTHON_SITE_PATH}" STREQUAL "")
  execute_process(COMMAND ${PYTHON_EXECUTABLE} -c "import site; print (site.getsitepackages()[0]);"
                  OUTPUT_VARIABLE PYTHON_SITE_PATH OUTPUT_STRIP_TRAILING_WHITESPACE)
endif()

message("-- Python site path at ${PYTHON_SITE_PATH}")
install(FILES
  "${PROJECT_BINARY_DIR}/cadabra2.so"
  "${PROJECT_SOURCE_DIR}/cadabra2_defaults.py" DESTINATION "${PYTHON_SITE_PATH}")

# Create the 'variable-substituted' cadabra2 inside the source tree so that we can
# run the tests and pick up cadabra2_defaults.py from there.

configure_file("${PROJECT_SOURCE_DIR}/cadabra2.in"             "${PROJECT_SOURCE_DIR}/cadabra2")

install(PROGRAMS "${PROJECT_SOURCE_DIR}/cadabra2" DESTINATION bin)

# Create Python dynamic library 'cadabra' without lib prefix.
include_directories(".")
pybind11_add_module(cadabra2 SHARED ${LOCAL_SRC_FILES})
target_link_libraries(cadabra2 PRIVATE ${PCRE_LIBRARIES} ${GMPXX_LIB} ${GMP_LIB} ${Boost_LIBRARIES})
#add_library(cadabra2 SHARED ${LOCAL_SRC_FILES})
#set_target_properties(cadabra2 PROPERTIES PREFIX "" SUFFIX ".so" IMPORT_SUFFIX ".so")
set_target_properties(cadabra2 PROPERTIES PUBLIC_HEADER "${PUBLIC_HEADER_FILES}")
add_executable(test_preprocessor test_preprocessor.cc PreProcessor.cc)
add_executable(test_multiindex   test_multiindex.cc)
add_executable(cadabra2python cadabra2python.cc CdbPython.cc)
target_link_libraries(cadabra2python ${PYTHON_LIBRARIES} ${Boost_LIBRARIES})
set_target_properties(cadabra2 PROPERTIES SUFFIX ".${PYTHON_MOD_SUFFIX}")
if(MATHEMATICA_FOUND)
  target_link_libraries(cadabra2 PRIVATE ${Mathematica_WSTP_LIBRARIES})
endif()

install(DIRECTORY DESTINATION bin DIRECTORY_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                                                        GROUP_READ GROUP_EXECUTE
                                                        WORLD_READ WORLD_EXECUTE)

install(DIRECTORY DESTINATION ${PYTHON_SITE_PATH} DIRECTORY_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                                                        GROUP_READ GROUP_EXECUTE
                                                        WORLD_READ WORLD_EXECUTE)



# Convert packages from cdb to python.
set(PACKAGES
  relativity/__init__
  relativity/schwarzschild
  gauge_theory/__init__
  gauge_theory/instantons
  )
set(PDIRS
  relativity
  gauge_theory
  )
set(PACKAGEDIR  ${CMAKE_BINARY_DIR}/core/packages/cdb)
message("-- Packages built in ${PACKAGEDIR}")
file(MAKE_DIRECTORY ${PACKAGEDIR})
foreach(PDIR ${PDIRS})
   add_custom_target("${PDIR}" ALL
	             COMMAND ${CMAKE_COMMAND} -E make_directory ${PACKAGEDIR}/${PDIR}
                     COMMENT "Creating package ${PDIR}...")
endforeach()

foreach(PACKAGE ${PACKAGES})
	string(REGEX REPLACE "/" "_" PACKAGETARGET ${PACKAGE})
	message("--    ${PACKAGETARGET}")					  
   add_custom_target("${PACKAGETARGET}" ALL
	                  COMMAND cadabra2python ${CMAKE_SOURCE_DIR}/core/packages/cdb/${PACKAGE}.cdb ${PACKAGEDIR}/${PACKAGE}.py
                     COMMENT "Creating ${PACKAGE}.py...")
endforeach()
																		
# Install packages.
set(PYTHON_CDB_PACKAGE_PATH ${PYTHON_SITE_PATH}/cdb)
foreach(PACKAGE ${PACKAGES})
  get_filename_component(PINSTALL ${PACKAGE} DIRECTORY)
  install(FILES "${PACKAGEDIR}/${PACKAGE}.py" DESTINATION ${PYTHON_CDB_PACKAGE_PATH}/${PINSTALL})
endforeach()

install(DIRECTORY DESTINATION ${PYTHON_CDB_PACKAGE_PATH} DIRECTORY_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                                                        GROUP_READ GROUP_EXECUTE
                                                        WORLD_READ WORLD_EXECUTE)

# Install conversion program.
install(TARGETS cadabra2python DESTINATION bin)

