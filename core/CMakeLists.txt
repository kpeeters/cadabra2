# cmake_minimum_required(VERSION ${CADABRA_CMAKE_VERSION}) # Required for various macros
project(Cadabra)
if(POLICY CMP0167)
  cmake_policy(SET CMP0167 NEW)
endif()

#---------------------------------------------------------------------------
# Preamble.
#---------------------------------------------------------------------------

print_header("Configuring core")

# configure_file(
#    "${PROJECT_SOURCE_DIR}/cadabra2.in"
#    "${PROJECT_BINARY_DIR}/cadabra2"
# )
configure_file(
   "${PROJECT_SOURCE_DIR}/cadabra2_defaults.py.in"
   "${PROJECT_BINARY_DIR}/packages/cadabra2_defaults.py"
)


#---------------------------------------------------------------------------
# Locate libraries.
#---------------------------------------------------------------------------
#SET(CMAKE_FIND_LIBRARY_SUFFIXES .a ${CMAKE_FIND_LIBRARY_SUFFIXES})

# Boost.regex, Boost.system
# if(ENABLE_JUPYTER)
#   # To avoid issues building the Jupyter kernel on Conda, link boost
#   # statically, to avoid version mismatch. Unfortunately, this cannot work,
#   # as you cannot link a static library into a shared one. But fortunately,
#   # for the Jupyter kernel, cadabra2.so only uses header-only Boost stuff,
#   # so no linking takes place
#   set(Boost_USE_STATIC_LIBS   ON)
# endif()
find_package(Boost 1.71.0 COMPONENTS system filesystem REQUIRED)
message(STATUS "Boost version ${Boost_VERSION}")

# Find glibmm (for base64)
if(USE_GTK4)
  find_package(GLIBMM4  REQUIRED)
else()
  find_package(GLIBMM3  REQUIRED)
endif()

# GMPXX
# In order to distribute as a PyPl wheel, we need to link
# statically; the line below picks up the right library,
# but then things fail because gmp/gmpxx are not built
# with -fPIC. Postponed for now.
# set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
#find_package(GMPXX REQUIRED STATIC)
find_package(GMPXX REQUIRED STATIC)

message(STATUS "Linking to GMP   library ${GMP_LIBRARIES}")
message(STATUS "Linking to GMPXX library ${GMPXX_LIBRARIES}")
message(STATUS "Include dir for GMP/GMPXX ${GMP_INCLUDE_DIRS}")

# Dependency of TinyProcessLib
find_package(Threads)


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
   algorithms/eliminate_vielbein.cc   
   algorithms/epsilon_to_delta.cc
   algorithms/evaluate.cc
   algorithms/expand.cc
   algorithms/expand_delta.cc
   algorithms/expand_diracbar.cc
   algorithms/expand_dummies.cc
   algorithms/expand_power.cc
   algorithms/explicit_indices.cc
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
   algorithms/meld.cc
   algorithms/nevaluate.cc
   algorithms/nval.cc
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
   algorithms/untrace.cc
   algorithms/vary.cc
   algorithms/young_project.cc
   algorithms/young_project_product.cc
   algorithms/young_project_tensor.cc
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
   properties/DerivativeOp.cc   
   properties/Determinant.cc
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
   properties/TableauInherit.cc   
   properties/TableauSymmetry.cc
   properties/Trace.cc  
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

set(MODULE_SRC_FILES
  pythoncdb/py_media.cc
  pythoncdb/py_ntensor.cc
  pythoncdb/py_algorithms.cc
  pythoncdb/py_ex.cc
  pythoncdb/py_globals.cc
  pythoncdb/py_helpers.cc
  pythoncdb/py_kernel.cc
  pythoncdb/py_module.cc
  pythoncdb/py_packages.cc
  pythoncdb/py_progress.cc
  pythoncdb/py_properties.cc
  pythoncdb/py_stopwatch.cc
  pythoncdb/py_tableau.cc
)

set(LOCAL_SRC_FILES
  NDSolver.cc
  NIntegrator.cc  
  NEvaluator.cc
  NTensor.cc
  NInterpolatingFunction.cc  
  InstallPrefix.cc
  DataCell.cc
  CdbPython.cc
  ExNode.cc
  ProgressMonitor.cc
  Bridge.cc
  Adjform.cc
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
  Hash.cc
  IndexIterator.cc
  IndexClassifier.cc
  Kernel.cc
  Linear.cc
  Media.cc
  Multiplier.cc
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
  ${CADABRA_LIBS_DIR}/whereami/whereami.c
  ${CADABRA_LIBS_DIR}/base64/base64.cc  
  ${ALGORITHM_SRC_FILES}
  ${PROPERTY_SRC_FILES}
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
   "${CADABRA_LIBS_DIR}/pybind11/include"
   "${CADABRA_LIBS_DIR}/internal/include"
   "${CADABRA_LIBS_DIR}/whereami"
   "${CADABRA_LIBS_DIR}/base64"   
   "${CADABRA_LIBS_DIR}/dbg"   
   "${CADABRA_LIBS_DIR}/linenoise"
   "${CADABRA_LIBS_DIR}/nlohmann"
   ${GMP_INCLUDE_DIRS}
   ${Boost_INCLUDE_DIRS}
   ${Python_INCLUDE_DIRS}
   )


#---------------------------------------------------------------------------
# Targets.
#---------------------------------------------------------------------------

# Cadabra2 python module
pybind11_add_module(cadabra2 SHARED
  ${LOCAL_SRC_FILES}
  ${MODULE_SRC_FILES}
)
set_target_properties(cadabra2 PROPERTIES 
  SUFFIX ".${Python_MOD_SUFFIX}" 
  CXX_VISIBILITY_PRESET default
)

target_link_libraries(cadabra2 PRIVATE
  ${GMPXX_LIBRARIES}
  ${GMP_LIBRARIES}   
  ${Boost_LIBRARIES}
  #  ${Python_LIBRARIES}
  #  ${GLIBMM_LIBRARIES}    
)
if(mimalloc_FOUND)
  target_link_libraries(cadabra2 PUBLIC mimalloc)
endif()

if(IPO_SUPPORTED)
  message(STATUS "IPO / LTO for cadabra2 enabled")     
  set_property(TARGET cadabra2 PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
  set_target_properties(cadabra2 PROPERTIES COMPILE_FLAGS "-flto=auto")
  set_target_properties(cadabra2 PROPERTIES LINK_FLAGS "-flto=auto")  
endif()

   
# On Unix, don't link the extension module to libpython*.so
# because some python executables are link to python static library for performance
# and if an extension is linked to python shared library, symbol clashes occur.
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set_target_properties(cadabra2 PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  target_link_libraries(cadabra2 PRIVATE ${Python_LIBRARIES})
elseif(CMAKE_SHARED_LINKER_FLAGS MATCHES ".*-Wl,--no-undefined.*")
  # Exception for systems that require no undefined symbols present
  # in shared libraries (e.g. openSUSE).
  message("-- Linking cadabra2.so to libpython.so because of default linker flags.")
  target_link_libraries(cadabra2 PRIVATE ${Python_LIBRARIES})
endif()

# Make the python module pick up elements from two parents up.
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set_target_properties(cadabra2 PROPERTIES INSTALL_RPATH "$ORIGIN/../../:$ORIGIN")

if(MATHEMATICA_FOUND)
   target_link_libraries(cadabra2 PRIVATE ${Mathematica_WSTP_LIBRARIES})
endif()

# cadabra2 CLI
add_executable(cadabra2-cli
   cadabra2-cli.cc
   CdbPython.cc
   Exceptions.cc
   InstallPrefix.cc
   pythoncdb/py_helpers.cc
   ${CADABRA_LIBS_DIR}/whereami/whereami.c
)
target_compile_definitions(cadabra2-cli PRIVATE CDBPYTHON_NO_NOTEBOOK)
target_include_directories(cadabra2-cli PRIVATE pybind11::pybind11)
target_link_libraries(cadabra2-cli PRIVATE pybind11::embed ${GLIBMM_LIBRARIES} ${Boost_LIBRARIES})

# Notebook diff tool
add_executable(cdb-nbtool
  cdb-nbtool.cc
  ${CADABRA_LIBS_DIR}/tiny-process-library/process.cpp
)
if(WIN32)
  target_sources(cdb-nbtool PRIVATE ${CADABRA_LIBS_DIR}/tiny-process-library/process_win.cpp)
else()
  target_sources(cdb-nbtool PRIVATE ${CADABRA_LIBS_DIR}/tiny-process-library/process_unix.cpp)
  target_link_libraries(cdb-nbtool PRIVATE Threads::Threads)
endif()
target_include_directories(cdb-nbtool PRIVATE ${CADABRA_LIBS_DIR}/tiny-process-library)

# Test preprocessor executable
add_executable(test_preprocessor 
   test_preprocessor.cc 
   PreProcessor.cc
   ${CADABRA_LIBS_DIR}/base64/base64.cc  
)

# cadabra2python executable
add_executable(cadabra2python 
   cadabra2python.cc 
   CdbPython.cc
   InstallPrefix.cc    
   DataCell.cc
   Exceptions.cc   
   ${CADABRA_LIBS_DIR}/whereami/whereami.c
   ${CADABRA_LIBS_DIR}/base64/base64.cc
   )
target_link_libraries(cadabra2python 
   ${Boost_LIBRARIES}
   #    ${GLIBMM_LIBRARIES}     
   ${Python_LIBRARIES}
   )

# cadabra2ipynb executable
add_executable(cadabra2ipynb
   cadabra2ipynb.cc 
   CdbPython.cc
   InstallPrefix.cc    
   DataCell.cc
   Exceptions.cc   
   ${CADABRA_LIBS_DIR}/whereami/whereami.c
   ${CADABRA_LIBS_DIR}/base64/base64.cc
   )
target_link_libraries(cadabra2ipynb
   ${Boost_LIBRARIES}
   #    ${GLIBMM_LIBRARIES}     
   ${Python_LIBRARIES}
   )

# cadabra2cadabra
add_executable(cadabra2cadabra 
   cadabra2cadabra.cc 
   DataCell.cc 
   InstallPrefix.cc
   Exceptions.cc
   ${CADABRA_LIBS_DIR}/whereami/whereami.c
   ${CADABRA_LIBS_DIR}/base64/base64.cc
   )
target_link_libraries(cadabra2cadabra
   ${Boost_LIBRARIES}
   )

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
# Tests run purely in C++ (no python).
#---------------------------------------------------------------------------

if(WIN32)
  message(STATUS "Not building C++ tests on Windows")
else()
  add_executable(test_internals
    test_internals.cc
  )
  add_executable(test_benchmark
    test_benchmark.cc
  )
  add_executable(test_multiplier
    test_multiplier.cc
    Multiplier.cc
  )
  # We are linking the test with the full python module, which is a bit
  # wasteful, but splitting out the ${LOCAL_SRC_FILES} into a separate
  # library turned out to lead to linking errors with the algorithm
  # modules in `packages/` (missing typeinfo things). For now this works.
  target_link_libraries(test_internals PRIVATE
    cadabra2
    ${GMPXX_LIBRARIES}
    ${GMP_LIBRARIES}
    ${Python_LIBRARIES}
  )
  target_link_libraries(test_benchmark PRIVATE
    cadabra2
    ${GMPXX_LIBRARIES}
    ${GMP_LIBRARIES}
    ${Python_LIBRARIES}
  )
  target_link_libraries(test_multiplier PRIVATE
    ${GMPXX_LIBRARIES}
    ${GMP_LIBRARIES}
  )
endif()


# Profiling mode
if(CMAKE_BUILD_TYPE STREQUAL "Profile")
  find_library(GPERFTOOLS_PROFILER profiler REQUIRED)
  message(STATUS "Building with profiling enabled: ${GPERFTOOLS_PROFILER}")
  set(CMAKE_CXX_FLAGS_PROFILE "${CMAKE_CXX_FLAGS_RELEASE} -g")
  target_link_libraries(cadabra2 PRIVATE -Wl,--no-as-needed ${GPERFTOOLS_PROFILER})
endif()

#---------------------------------------------------------------------------
# Packages.
#---------------------------------------------------------------------------

add_subdirectory(packages)


#---------------------------------------------------------------------------
# Installation.
#---------------------------------------------------------------------------

# Python module
# 
# https://stackoverflow.com/questions/21198030/installfiles-cmake-cfg-intdir-abc-win-dll-destination-bin

install_directory_permissions(${PYTHON_SITE_PATH})

if(WIN32)
  install(
    TARGETS
    cadabra2
    DESTINATION 
    .
    )
else()
  install(
    TARGETS
    cadabra2
    DESTINATION 
    ${PYTHON_SITE_PATH}
    )
endif()

if(WIN32)
  # Install the Python runtime packages.
  set(PYLIBDIR "/${MSYS_ENV}/lib/python${Python_VERSION_MAJOR}.${Python_VERSION_MINOR}")
  execute_process(
    COMMAND         cygpath -m ${PYLIBDIR}
    OUTPUT_VARIABLE WINPYTHON
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  message(STATUS "Copying python runtime from ${PYLIBDIR} aka ${WINPYTHON}")
  install(DIRECTORY ${WINPYTHON} DESTINATION lib)
endif()

install(TARGETS cadabra2-cli DESTINATION ${CDB_BIN_PATH})
install(TARGETS cdb-nbtool DESTINATION ${CDB_BIN_PATH})
message(STATUS "Triple-check binary install path ${CMAKE_INSTALL_PREFIX}/${CDB_BIN_PATH}")
if(NOT WIN32)
  # Windows cmake followed by wix leads to symbolic link issues, so don't.
  install(CODE "
    set(CDB_BIN_PATH \"${CDB_BIN_PATH}\")

    # Remove old binary if it exists
    file(REMOVE \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/\${CDB_BIN_PATH}/cadabra2\")
    
    # Create symbolic link
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E create_symlink cadabra2-cli cadabra2
        WORKING_DIRECTORY \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/\${CDB_BIN_PATH}\"
    )
   ")
endif()
install(
   FILES 
      "${PROJECT_BINARY_DIR}/packages/cadabra2_defaults.py" 
      "${CADABRA_LIBS_DIR}/appdirs/cdb_appdirs.py"
   DESTINATION 
      "${PYTHON_SITE_PATH}"
)

# CLI
# install(
#   PROGRAMS 
#     "${PROJECT_BINARY_DIR}/cadabra2" 
#   DESTINATION 
#     ${CDB_BIN_PATH}
# )

# cadabra2python
install(
  TARGETS 
    cadabra2python 
  DESTINATION 
    ${CDB_BIN_PATH}
)
install_directory_permissions("bin")

# cadabra2ipynb
install(
  TARGETS 
    cadabra2ipynb
  DESTINATION 
    ${CDB_BIN_PATH}
)

# cadabra2cadabra
install(
   TARGETS 
   cadabra2cadabra
   DESTINATION 
   ${CDB_BIN_PATH}
 )

if(NOT WIN32)
   if(CMAKE_GENERATOR MATCHES "Ninja")
      set(destdir_expr "\$\${DESTDIR}")
   else()
      set(destdir_expr "\$(DESTDIR)")
   endif()
endif()             

# manual pages
if(NOT WIN32)
  install(
    FILES
    ../man/man1/cadabra2.1
    ../man/man1/cadabra2-cli.1    
    ../man/man1/cadabra2cadabra.1
    ../man/man1/cadabra2ipynb.1
    ../man/man1/cadabra2html.1 
    ../man/man1/cadabra2latex.1
    ../man/man1/cadabra2python.1  
    DESTINATION
    share/man/man1
    )
endif()

if(APPIMAGE_MODE)
  # Install sympy. Matplotlib would be nice, but that pulls in blas and fortran and zillions of other things,
  # which then don't link properly anymore either.
  install(CODE      "execute_process(COMMAND echo \"Installing python things in \$ENV{DESTDIR}|${CMAKE_INSTALL_PREFIX}|${PYTHON_SITE_PATH}\")")
  install(CODE      "execute_process(COMMAND pip3 install --target \$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/${PYTHON_SITE_PATH} sympy matplotlib)")
endif()
