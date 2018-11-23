cmake_minimum_required(VERSION 3.1) # Required for various macros
project(Cadabra)


#---------------------------------------------------------------------------
# Preamble.
#---------------------------------------------------------------------------

print_header("Configuring core")

configure_file(
	"${PROJECT_SOURCE_DIR}/cadabra2.in"
	"${PROJECT_SOURCE_DIR}/cadabra2"
)


#---------------------------------------------------------------------------
# Locate libraries.
#---------------------------------------------------------------------------

# Boost.regex, Boost.system
find_package(Boost 1.53 COMPONENTS system)

# Find glibmm (for base64)
find_package(GLIBMM  REQUIRED)

# GMPXX
find_package(GMPXX REQUIRED)

#---------------------------------------------------------------------------
# Enumerate input files and directories.
#---------------------------------------------------------------------------

SET(ALGORITHM_SRC_FILES
	algorithms/canonicalise.cc
	algorithms/collect_components.cc
	algorithms/collect_factors.cc
	algorithms/collect_terms.cc
	algorithms/combine.cc
	algorithms/complete.cc
	algorithms/decompose.cc
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
	algorithms/lower_free_indices.cc
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
	algorithms/tab_dimension.cc  
	algorithms/tab_basics.cc
	algorithms/take_match.cc
	algorithms/replace_match.cc
	algorithms/unwrap.cc
	algorithms/unzoom.cc    
	algorithms/vary.cc
	algorithms/young_project.cc
	algorithms/young_project_product.cc
	algorithms/young_project_tensor.cc
	algorithms/young_reduce.cc
	algorithms/zoom.cc  
)

set(PROPERTY_SRC_FILES
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
	properties/Vielbein.cc
	properties/Weight.cc
	properties/WeightInherit.cc
	properties/WeylTensor.cc
)

# Packages are now handled by a CMakeLists.txt in the
# packages directory.
#
# set(PACKAGES
#   core/manip
#   relativity/__init__
#   relativity/schwarzschild
#   gauge_theory/__init__
#   gauge_theory/instantons
#   )

set(LOCAL_SRC_FILES
	CdbPython.cc
	PythonCdb.cc
	ExNode.cc
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
	IndexClassifier.cc
	Kernel.cc
	Linear.cc
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
	modules/xperm_new.cc
	${ALGORITHM_SRC_FILES}
	${PROPERTY_SRC_FILES}
	${CADABRA_LIBS_DIR}/jsoncpp/jsoncpp.cpp
)
if(MATHEMATICA_FOUND)
	set(LOCAL_SRC_FILES 
		${LOCAL_SRC_FILES}
		MMACdb.cc  
		algorithms/map_mma.cc
	)
endif()

set(IMAGES
	../images/cadabra.png
)


#---------------------------------------------------------------------------
# Include directories and preprocessor definitions.
#---------------------------------------------------------------------------

include_directories(
	"."
	"${CADABRA_LIBS_DIR}/internal/include"
	"${CADABRA_LIBS_DIR}/whereami"
	"${CADABRA_LIBS_DIR}/jsoncpp"
	${Boost_INCLUDE_DIRS}
  )
# FIXME: 2nd line is to find Config.hh
# FIXME: the client_server bit should go back to client_server eventually


#---------------------------------------------------------------------------
# Targets.
#---------------------------------------------------------------------------

# Cadabra2 python module
pybind11_add_module(cadabra2 SHARED 
	${LOCAL_SRC_FILES}
)
set_target_properties(cadabra2 PROPERTIES SUFFIX ".${PYTHON_MOD_SUFFIX}")
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
	${CADABRA_LIBS_DIR}/jsoncpp/jsoncpp.cpp 
)
target_link_libraries(cadabra2python 
	${Boost_LIBRARIES} 
	${PYTHON_LIBRARIES}
)

# cadabra2cadabra
add_executable(cadabra2cadabra 
	cadabra2cadabra.cc 
	DataCell.cc 
	InstallPrefix.cc
	${CADABRA_LIBS_DIR}/jsoncpp/jsoncpp.cpp 
	${CADABRA_LIBS_DIR}/whereami/whereami.c
)
target_link_libraries(cadabra2cadabra ${Boost_LIBRARIES}	${GLIBMM3_LIBRARIES})

# Mathematica WSTP test
if(MATHEMATICA_FOUND)
	message(STATUS "Building with Mathematica support (linking against ${Mathematica_VERSION})")
	include_directories(${Mathematica_WSTP_INCLUDE_DIR})
	add_executable(test_wstp         test_wstp.cc)
	target_link_libraries(test_wstp ${Mathematica_WSTP_LIBRARIES})
else()
	message(STATUS "Building without Mathematica support")
endif()


#---------------------------------------------------------------------------
# Installation.
#---------------------------------------------------------------------------

# Python module
# 
# https://stackoverflow.com/questions/21198030/installfiles-cmake-cfg-intdir-abc-win-dll-destination-bin

if(NOT WIN32)
	# Remove any old cadabra2.so files in the global site-wide path which may have been
	# left there from a previous install.
	remove_file("${OLD_PYTHON_SITE_PATH}/cadabra2.${PYTHON_MOD_SUFFIX}")
	remove_file("${OLD_PYTHON_SITE_PATH}/cadabra2_defaults.py")
endif()

install_directory_permissions(${PYTHON_SITE_PATH})

if (MSVC)
  install(
	 TARGETS
	 cadabra2
	 DESTINATION 
	 bin
	 )
else()
  install(
	 TARGETS
	 cadabra2
	 DESTINATION 
	 ${PYTHON_SITE_PATH}
	 )
endif()

install(
	FILES 
		"${PROJECT_SOURCE_DIR}/cadabra2_defaults.py" 
		"${CADABRA_LIBS_DIR}/appdirs/cdb_appdirs.py"
	DESTINATION 
		"${PYTHON_SITE_PATH}"
)

# CLI
install(
  PROGRAMS 
    "${PROJECT_SOURCE_DIR}/cadabra2" 
  DESTINATION 
    bin
)
install_directory_permissions("bin")

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

if (MSVC AND NOT INSTALL_TARGETS_ONLY)
	install_dlls_from("core")
endif()
