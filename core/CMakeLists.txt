cmake_minimum_required(VERSION 3.1) # Required for various macros
project(Cadabra)
message("------------------------------------------------------------------------\nConfiguring 'core'")


#---------------------------------------------------------------------------
# Preamble.
#---------------------------------------------------------------------------

set(PYTHON_CDB_PACKAGE_PATH ${PYTHON_SITE_PATH}/cdb)
set(PACKAGEDIR  ${CMAKE_BINARY_DIR}/core/packages/cdb)

configure_file(
  "${PROJECT_SOURCE_DIR}/cadabra2.in"
  "${PROJECT_SOURCE_DIR}/cadabra2"
)


#---------------------------------------------------------------------------
# Locate libraries.
#---------------------------------------------------------------------------

# Boost.regex, Boost.system
find_package(Boost 1.53 COMPONENTS system)

# GMPXX
find_package(GMPXX REQUIRED)
message("-- Found gmpxx  ${GMPXX_LIBRARIES}")
message("-- Found gmp    ${GMP_LIBRARIES}")
message("-- Found python ${PYTHON_LIBRARIES}")

#---------------------------------------------------------------------------
# Enumerate input files and directories.
#---------------------------------------------------------------------------

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
  algorithms/simplify.cc
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

set(IMAGES
  ../images/cadabra.png
)

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


#---------------------------------------------------------------------------
# Include directories and preprocessor definitions.
#---------------------------------------------------------------------------

include_directories(
  "."
  "../libs/internal/include"
  "../client_server/jsoncpp"
  ${Boost_INCLUDE_DIRS}
  )
# FIXME: 2nd line is to find Config.hh
# FIXME: the client_server bit should go back to client_server eventually

add_definitions(
  -DNOMINMAX
#  -DBOOST_ALL_NO_LIB
)

#---------------------------------------------------------------------------
# Targets.
#---------------------------------------------------------------------------

# Cadabra2 python module
pybind11_add_module(cadabra2 SHARED 
  ${LOCAL_SRC_FILES}
)
set_target_properties(cadabra2 
  PROPERTIES 
	SUFFIX ".${PYTHON_MOD_SUFFIX}" 
)
target_link_libraries(cadabra2 PRIVATE
  ${GMPXX_LIBRARIES}
  ${GMP_LIBRARIES}   
  ${Boost_LIBRARIES} 
)
if(MATHEMATICA_FOUND)
  target_link_libraries(cadabra2 PRIVATE ${Mathematica_WSTP_LIBRARIES})
endif()

# Test preprocessor executable
add_executable(test_preprocessor 
  test_preprocessor.cc 
  PreProcessor.cc
)

# cadabra2python executable
add_executable(cadabra2python 
  cadabra2python.cc 
  CdbPython.cc
)
target_link_libraries(cadabra2python 
  ${Boost_LIBRARIES} 
  ${PYTHON_LIBRARIES}
  )

# cadabra2cadabra
add_executable(cadabra2cadabra cadabra2cadabra.cc DataCell.cc ../client_server/jsoncpp/jsoncpp.cpp)
target_link_libraries(cadabra2cadabra ${Boost_LIBRARIES})

# Mathematica WSTP test
if(MATHEMATICA_FOUND)
  message("-- Building with Mathematica support (linking against ${Mathematica_VERSION})")
  include_directories(${Mathematica_WSTP_INCLUDE_DIR})
  add_executable(test_wstp         test_wstp.cc)
  target_link_libraries(test_wstp ${Mathematica_WSTP_LIBRARIES})
else()
  message("-- Building without Mathematica support")
endif()

# Convert packages from cdb to python.
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
																		

#---------------------------------------------------------------------------
# Installation.
#---------------------------------------------------------------------------

# Python module
# 
# https://stackoverflow.com/questions/21198030/installfiles-cmake-cfg-intdir-abc-win-dll-destination-bin
if(WIN32)
  set(CADABRA_BUILD_TYPE "${CMAKE_BUILD_TYPE}")
  if(${CADABRA_BUILD_TYPE}=="")
	 set(CADABRA_BUILD_TYPE "Debug")
  endif()
  install(
	 FILES
    "${PROJECT_BINARY_DIR}/${CADABRA_BUILD_TYPE}/cadabra2.${PYTHON_MOD_SUFFIX}"
    "${PROJECT_SOURCE_DIR}/cadabra2_defaults.py" 
    "${PROJECT_BINARY_DIR}/${CADABRA_BUILD_TYPE}/mpir.dll"
	 DESTINATION 
    "${PYTHON_SITE_PATH}"
	 )
else()
  install(
	 FILES
    "${PROJECT_BINARY_DIR}/cadabra2.${PYTHON_MOD_SUFFIX}"
    "${PROJECT_SOURCE_DIR}/cadabra2_defaults.py" 
	 DESTINATION 
    "${PYTHON_SITE_PATH}"
	 )
endif()
install(
  DIRECTORY DESTINATION 
    ${PYTHON_SITE_PATH} 
  DIRECTORY_PERMISSIONS 
    OWNER_READ 
	OWNER_WRITE 
	OWNER_EXECUTE
    GROUP_READ 
	GROUP_EXECUTE
    WORLD_READ 
	WORLD_EXECUTE
)

# CLI
install(
  PROGRAMS 
    "${PROJECT_SOURCE_DIR}/cadabra2" 
  DESTINATION 
    bin
)
install(
  DIRECTORY DESTINATION 
    bin 
  DIRECTORY_PERMISSIONS 
    OWNER_READ 
	OWNER_WRITE 
	OWNER_EXECUTE
	GROUP_READ
	GROUP_EXECUTE
	WORLD_READ
	WORLD_EXECUTE
)

# Packages
foreach(PACKAGE ${PACKAGES})
  get_filename_component(PINSTALL ${PACKAGE} DIRECTORY)
  install(
    FILES 
      "${PACKAGEDIR}/${PACKAGE}.py" 
    DESTINATION 
      ${PYTHON_CDB_PACKAGE_PATH}/${PINSTALL}
  )
endforeach()
install(
  DIRECTORY DESTINATION 
    ${PYTHON_CDB_PACKAGE_PATH} 
  DIRECTORY_PERMISSIONS 
    OWNER_READ 
	OWNER_WRITE 
	OWNER_EXECUTE
	GROUP_READ 
	GROUP_EXECUTE
    WORLD_READ 
	WORLD_EXECUTE
)

# cadabra2python
install(
  TARGETS 
    cadabra2python 
  DESTINATION 
    bin
)

# cadabra2cadabra
install(
  TARGETS 
    cadabra2cadabra
  DESTINATION 
    bin
)

# Shared libraries
# install(SCRIPT "${CMAKE_SOURCE_DIR}/cmake/core_install_dlls.cmake")

