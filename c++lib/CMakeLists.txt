cmake_minimum_required(VERSION 3.12)

set(CMAKE_CXX_STANDARD 17)
project(Cadabra)

#---------------------------------------------------------------------------
# Preamble.
#---------------------------------------------------------------------------

# Set path to additional cmake files
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../cmake/modules")

# Disable warning 'MACOSX_RPATH is enabled by default'
if (POLICY CMP0042)
  cmake_policy(SET CMP0042 NEW)
endif(POLICY CMP0042)

# Disable warning 'Only interpret if() arguments as variables or keywords when unquoted'
if (POLICY CMP0054)
  cmake_policy(SET CMP0054 NEW)
endif()

# Ensure that we can build the library and install it without having to
# build the samples.
set(CMAKE_SKIP_INSTALL_ALL_DEPENDENCY TRUE)

find_package(Python REQUIRED COMPONENTS Interpreter Development)
message(STATUS "Found python library: ${Python_LIBRARIES}")
message(STATUS "Found python headers: ${Python_INCLUDE_DIRS}")
message(STATUS "Python version is ${Python_VERSION_MAJOR}.${Python_VERSION_MINOR}.")

#---------------------------------------------------------------------------
# User options.
#---------------------------------------------------------------------------

set(INSTALL_LIB_DIR     lib                CACHE PATH "Installation directory for library")
set(INSTALL_INCLUDE_DIR include/cadabra2++ CACHE PATH "Installation directory for header files")

# Scalar backend options
option(USE_TREETRACKER "Use the TreeTracker scalar backend" OFF)
set(PATH_TREETRACKER   "./treetracker" CACHE STRING "Path to the TreeTracker library")


#---------------------------------------------------------------------------
# Compiler flags.
#---------------------------------------------------------------------------

add_definitions("-DNO_SYMPY")
if(CMAKE_COMPILER_IS_GNUCXX)
  add_definitions("-Wall -g -Wno-unused-but-set-variable")
endif()

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")
endif()

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")
endif()

if(MSVC)
	set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
	set(MSVC_FLAGS
		"/wd4101"						# unreferenced local variable
		"/wd4250"						# inherits via dominance
		"/wd4244"						# conversion from x to y, possible loss of data
		"/wd4267"						# same as 4244
		"/wd4996"						# deprecated POSIX functions
		"-D_CRT_SECURE_NO_WARNINGS"		# don't warn about deprecated functions
	)
	foreach(FLAG ${MSVC_FLAGS})
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${FLAG}")
	endforeach()

endif()


#---------------------------------------------------------------------------
# Find libraries.
#---------------------------------------------------------------------------

# Locate gmpxx. On Homebrew there seems to be constantly something wrong with
# the pkgconfig for gmpxx. So we just add the include path by hand.
if(APPLE)
   add_definitions("-I/usr/local/include -I/opt/local/include")
endif()
if(MSVC)
  find_package(GMPXX REQUIRED)
  set(GMP_LIB "${GMPXX_LIBRARIES}")
  set(GMPXX_LIB "${GMPXX_LIBRARIES}")
  include_directories("${VCPKG_INCLUDE_DIRS}")
else()
  find_library(GMP_LIB gmp REQUIRED)
  find_library(GMPXX_LIB gmpxx REQUIRED)
  message(STATUS "Found gmp ${GMP_LIB}")
  message(STATUS "Found gmpxx ${GMPXX_LIB}")
endif()


#---------------------------------------------------------------------------
# Enumerate source files.
#---------------------------------------------------------------------------

set(LOCAL_SRC_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/Adjform.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/Algorithm.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/Cleanup.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/Combinatorics.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/Compare.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/DisplayBase.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/DisplayTeX.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/DisplaySympy.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/DisplayTerminal.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/TerminalStream.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/NEvaluator.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/NTensor.cc  
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/Exceptions.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/Exchange.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/Functional.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/IndexIterator.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/IndexClassifier.cc  
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/Hash.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/Kernel.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/Parser.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/PreClean.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/PreProcessor.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/ProgressMonitor.cc  
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/Props.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/Stopwatch.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/Storage.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/Symbols.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/YoungTab.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/properties/Accent.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/properties/AntiCommuting.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/properties/AntiSymmetric.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/properties/Commuting.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/properties/CommutingAsProduct.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/properties/CommutingAsSum.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/properties/CommutingBehaviour.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/properties/Coordinate.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/properties/DAntiSymmetric.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/properties/Depends.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/properties/DependsInherit.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/properties/Derivative.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/properties/DerivativeOp.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/properties/Determinant.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/properties/Diagonal.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/properties/DifferentialForm.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/properties/DiracBar.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/properties/Distributable.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/properties/EpsilonTensor.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/properties/ExteriorDerivative.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/properties/FilledTableau.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/properties/GammaMatrix.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/properties/GammaTraceless.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/properties/ImaginaryI.cc  
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/properties/ImplicitIndex.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/properties/Indices.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/properties/Integer.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/properties/InverseMetric.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/properties/KroneckerDelta.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/properties/LaTeXForm.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/properties/Matrix.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/properties/Metric.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/properties/NonCommuting.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/properties/NumericalFlat.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/properties/PartialDerivative.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/properties/RiemannTensor.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/properties/SatisfiesBianchi.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/properties/SelfAntiCommuting.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/properties/SelfCommuting.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/properties/SelfNonCommuting.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/properties/SortOrder.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/properties/Spinor.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/properties/Symbol.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/properties/Symmetric.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/properties/Tableau.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/properties/TableauBase.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/properties/TableauInherit.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/properties/TableauSymmetry.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/properties/Trace.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/properties/Traceless.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/properties/Weight.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/properties/WeightInherit.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/properties/WeylTensor.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/modules/xperm_new.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/algorithms/canonicalise.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/algorithms/collect_components.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/algorithms/collect_factors.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/algorithms/collect_terms.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/algorithms/combine.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/algorithms/complete.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/algorithms/decompose_product.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/algorithms/distribute.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/algorithms/drop_weight.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/algorithms/einsteinify.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/algorithms/eliminate_kronecker.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/algorithms/eliminate_metric.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/algorithms/epsilon_to_delta.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/algorithms/evaluate.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/algorithms/expand.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/algorithms/expand_delta.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/algorithms/expand_diracbar.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/algorithms/expand_power.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/algorithms/factor_in.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/algorithms/factor_out.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/algorithms/fierz.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/algorithms/flatten_product.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/algorithms/flatten_sum.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/algorithms/indexsort.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/algorithms/integrate_by_parts.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/algorithms/join_gamma.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/algorithms/keep_terms.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/algorithms/lr_tensor.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/algorithms/order.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/algorithms/product_rule.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/algorithms/reduce_delta.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/algorithms/rename_dummies.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/algorithms/rewrite_indices.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/algorithms/simplify.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/algorithms/sort_product.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/algorithms/sort_sum.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/algorithms/split_gamma.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/algorithms/split_index.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/algorithms/substitute.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/algorithms/sym.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/algorithms/tab_basics.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/algorithms/take_match.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/algorithms/replace_match.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/algorithms/unwrap.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/algorithms/vary.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/algorithms/young_project.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/algorithms/young_project_product.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/algorithms/young_project_tensor.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/algorithms/meld.cc
  SympyDummy.cc
)
set(MAIN_HEADERS
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/Adjform.hh
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/Algorithm.hh
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/Combinatorics.hh
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/Compare.hh
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/DisplayBase.hh
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/DisplayTerminal.hh
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/NEvaluator.hh
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/Exceptions.hh
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/IndexClassifier.hh
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/IndexIterator.hh
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/Kernel.hh
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/lru_cache.hh
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/Hash.hh
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/NTensor.hh  
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/Parser.hh
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/ProgressMonitor.hh
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/Props.hh
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/Storage.hh
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/Stopwatch.hh
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/TerminalStream.hh
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/tree.hh
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/YoungTab.hh
  )
FILE(GLOB ALGO_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/../core/algorithms/*.hh)
FILE(GLOB PROP_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/../core/properties/*.hh)

# Copy relevant header files to the binary directory
file(COPY ${MAIN_HEADERS} DESTINATION ${CMAKE_BINARY_DIR}/include/cadabra2++)
file(COPY ${ALGO_HEADERS} DESTINATION ${CMAKE_BINARY_DIR}/include/cadabra2++/algorithms)
file(COPY ${PROP_HEADERS} DESTINATION ${CMAKE_BINARY_DIR}/include/cadabra2++/properties)

# Create a 'master' header which includes all the above for convenience
file(WRITE "${CMAKE_BINARY_DIR}/include/cadabra2++.hh" "// Main headers\n")
foreach(HEADER ${MAIN_HEADERS})
	get_filename_component(FILENAME "${HEADER}" NAME)
	file(APPEND "${CMAKE_BINARY_DIR}/include/cadabra2++.hh" "#include \"cadabra2++/${FILENAME}\"\n")
endforeach()
file(APPEND "${CMAKE_BINARY_DIR}/include/cadabra2++.hh" "\n// Properties\n")
foreach(HEADER ${PROP_HEADERS})
	get_filename_component(FILENAME "${HEADER}" NAME)
	file(APPEND "${CMAKE_BINARY_DIR}/include/cadabra2++.hh" "#include \"cadabra2++/properties/${FILENAME}\"\n")
endforeach()
file(APPEND "${CMAKE_BINARY_DIR}/include/cadabra2++.hh" "\n// Algorithms\n")
foreach(HEADER ${ALGO_HEADERS})
	get_filename_component(FILENAME "${HEADER}" NAME)
	file(APPEND "${CMAKE_BINARY_DIR}/include/cadabra2++.hh" "#include \"cadabra2++/algorithms/${FILENAME}\"\n")
endforeach()
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/cpplib.hh.in CPPLIB_HH_IN)
file(APPEND "${CMAKE_BINARY_DIR}/include/cadabra2++.hh" "\n//Helper functions\n${CPPLIB_HH_IN}")

set(LIB_INCLUDE_DIRS
	"." 
	"${CADABRA_CORE_DIR}"
	"${CADABRA_LIBS_DIR}/pybind11/include"
	"${CADABRA_LIBS_DIR}/internal/include"
	"${CADABRA_LIBS_DIR}/dbg"
	${Python_INCLUDE_DIRS}
)
message("${LIB_INCLUDE_DIRS}")

if(USE_TREETRACKER)
  add_definitions(-DUSE_TREETRACKER)
  if(EXISTS "${PATH_TREETRACKER}/treetracker.h" OR EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${PATH_TREETRACKER}/treetracker.h")
	 include_directories(${PATH_TREETRACKER})
	 message("-- Using the TreeTracker scalar backend at ${PATH_TREETRACKER}")
  else()
	 message(FATAL_ERROR "-- Cannot find the 'treetracker.h' file in '${PATH_TREETRACKER}'")
  endif()
else()
  message("-- Not using the TreeTracker scalar backend")
endif()


#---------------------------------------------------------------------------
# Targets
#---------------------------------------------------------------------------

add_library(cadabra2++objects         OBJECT ${LOCAL_SRC_FILES})
set_property(TARGET cadabra2++objects PROPERTY POSITION_INDEPENDENT_CODE 1)
target_include_directories(cadabra2++objects PUBLIC "${LIB_INCLUDE_DIRS}")

add_library(cadabra2++         SHARED $<TARGET_OBJECTS:cadabra2++objects>)
set_target_properties(cadabra2++        PROPERTIES PUBLIC_HEADER "${PUBLIC_HEADER_FILES}")

target_link_libraries(cadabra2++        ${GMPXX_LIB} ${GMP_LIB} ${Python_LIBRARIES})

add_library(cadabra2++_static  STATIC $<TARGET_OBJECTS:cadabra2++objects>)
target_link_libraries(cadabra2++_static ${GMPXX_LIB} ${GMP_LIB} ${Python_LIBRARIES})

# Trivial example
add_executable(trivial trivial.cc)
target_include_directories(trivial PUBLIC
  "${CADABRA_LIBS_DIR}/pybind11/include"
  "${CMAKE_BINARY_DIR}/include"
  "${CMAKE_BINARY_DIR}/include/cadabra2++"
  ${Python_INCLUDE_DIRS}
)
target_link_libraries(trivial cadabra2++)

# Sample executable
add_executable(simple simple.cc)
target_include_directories(simple PUBLIC
  "${CADABRA_LIBS_DIR}/pybind11/include"
  "${CMAKE_BINARY_DIR}/include"
  "${CMAKE_BINARY_DIR}/include/cadabra2++"
  ${Python_INCLUDE_DIRS}
)
target_link_libraries(simple cadabra2++)
 
# Test for adjform
add_executable(adjform adjform.cc)
target_include_directories(adjform PUBLIC
  "${CADABRA_LIBS_DIR}/pybind11/include"
  "${CMAKE_BINARY_DIR}/include"
  "${CMAKE_BINARY_DIR}/include/cadabra2++"
  ${Python_INCLUDE_DIRS}
)
target_link_libraries(adjform cadabra2++)
 
# Test for numerical evaluation
add_executable(nevaluate nevaluate.cc)
target_include_directories(nevaluate PUBLIC
  "${CADABRA_LIBS_DIR}/pybind11/include"
  "${CMAKE_BINARY_DIR}/include"
  "${CMAKE_BINARY_DIR}/include/cadabra2++"
  ${Python_INCLUDE_DIRS}
)
target_link_libraries(nevaluate cadabra2++)
 

#---------------------------------------------------------------------------
# Installation
#---------------------------------------------------------------------------

install(TARGETS  cadabra2++        LIBRARY DESTINATION "${INSTALL_LIB_DIR}")
install(TARGETS  cadabra2++_static LIBRARY DESTINATION "${INSTALL_LIB_DIR}" ARCHIVE DESTINATION "${INSTALL_LIB_DIR}")
install(DIRECTORY ${CMAKE_BINARY_DIR}/include DESTINATION "${INSTALL_INCLUDE_DIR}")

install(DIRECTORY 
	DESTINATION bin 
	DIRECTORY_PERMISSIONS 
		OWNER_READ 
		OWNER_WRITE 
		OWNER_EXECUTE
		GROUP_READ 
		GROUP_EXECUTE
		WORLD_READ 
		WORLD_EXECUTE
)

install(DIRECTORY 
	DESTINATION lib
	DIRECTORY_PERMISSIONS 
		OWNER_READ 
		OWNER_WRITE 
		OWNER_EXECUTE
		GROUP_READ 
		GROUP_EXECUTE
		WORLD_READ 
		WORLD_EXECUTE
)
