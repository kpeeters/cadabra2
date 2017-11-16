cmake_minimum_required(VERSION 2.8)

project(CadabraTests)

message("-- Configuring self-tests")

add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure)

set(PYTHONPATH ${CMAKE_SOURCE_DIR}/core:${CMAKE_BINARY_DIR}/core/packages:${CMAKE_BINARY_DIR}/core)
message("-- Set PYTHONPATH for testing to ${PYTHONPATH}:$ENV{PYTHONPATH}")

if(USE_PYTHON_3)
  set(Python_ADDITIONAL_VERSIONS 3.4 3.5 3.6)
  set(PythonInterp_ADDITIONAL_VERSIONS 3.4 3.5 3.6)
  find_package(PythonInterp 3.4 REQUIRED)
  if(APPLE)
     find_package(PythonLibsOSX 3.4 REQUIRED)
  else()
     find_package(PythonLibs 3.4 REQUIRED)
  endif()
else()
  set(Python_ADDITIONAL_VERSIONS 2.7)
  set(PythonInterp_ADDITIONAL_VERSIONS 2.7)
  find_package(PythonInterp 2.7 REQUIRED)
  if(APPLE)
     find_package(PythonLibsOSX 2.7 REQUIRED)
  else()
     find_package(PythonLibs 2.7 REQUIRED)
  endif()
endif()
message("-- Python version = ${PYTHONLIBS_VERSION_STRING}")

if (!WIN32)
  add_test(comparison     test_comparison)
endif()

# Locate Boost and Boost.Python.
add_definitions("-DBOOST_ALL_NO_LIB")
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
if(USE_PYTHON_3)
  find_package(Boost 1.53.0 COMPONENTS python3 regex)
  if(NOT Boost_PYTHON3_FOUND)
    if("${PYTHONLIBS_VERSION_STRING}" MATCHES "^3.4")
      find_package(Boost 1.53.0 COMPONENTS python-py34 regex REQUIRED)
    else()
	  if("${PYTHONLIBS_VERSION_STRING}" MATCHES "^3.5")    
	    find_package(Boost 1.53.0 COMPONENTS python-py35 regex REQUIRED)
	  else()
		find_package(Boost 1.53.0 COMPONENTS python-py36 regex REQUIRED)
	  endif()
    endif()
  endif()
else()
  find_package(Boost 1.53.0 COMPONENTS python regex)
endif()

foreach(LIBARRAY ${Boost_LIBRARIES})
	get_filename_component(BOOST_ANOTHER_PATH_DIR ${LIBARRAY} DIRECTORY)
	list(APPEND BOOST_EXTRA_PATH ${BOOST_ANOTHER_PATH_DIR})
endforeach()
if(BOOST_EXTRA_PATH)
	list(REMOVE_DUPLICATES BOOST_EXTRA_PATH)
endif()
message(" ---- boostextrapath was ${BOOST_EXTRA_PATH}")

# Notebooks with built-in tests.
#
set(CNBIN   ${CMAKE_SOURCE_DIR}/examples)
set(CDBOUT  ${CMAKE_BINARY_DIR}/tests)
set(DC      ${CMAKE_SOURCE_DIR}/client_server/cadabra2html.cc ${CMAKE_SOURCE_DIR}/client_server/DataCell.hh ${CMAKE_SOURCE_DIR}/client_server/DataCell.cc)
set(NBTESTS
  quickstart
  beginners
  tensor_monomials
  for_previous_users
  converge
  fierz
  gamma_matrix_algebra
  poincare_algebra
  string_states
  bianchi_identities
  schwarzschild
  kaluza_klein
  scalar_manipulations
  )
foreach(NBTEST ${NBTESTS})
  if(WIN32)
    add_custom_command(OUTPUT  ${CDBOUT}/${NBTEST}.cdb
	     DEPENDS ${CNBIN}/${NBTEST}.cnb ${DC}
	     COMMAND set PATH=${BOOST_EXTRA_PATH}\;%PATH%
	     COMMAND cadabra2cadabra
	     ARGS    ${CNBIN}/${NBTEST}.cnb ${CDBOUT}/${NBTEST}.cdb
	     COMMENT "Creating ${NBTEST} notebook...")
  else()
    add_custom_command(OUTPUT  ${CDBOUT}/${NBTEST}.cdb
	     DEPENDS ${CNBIN}/${NBTEST}.cnb ${DC}
	     COMMAND cadabra2cadabra
	     ARGS    ${CNBIN}/${NBTEST}.cnb ${CDBOUT}/${NBTEST}.cdb
	     COMMENT "Creating ${NBTEST} notebook...")
  endif()
  add_custom_target("${NBTEST}_test" ALL echo -n DEPENDS ${CDBOUT}/${NBTEST}.cdb)
  add_test(${NBTEST}  ${CMAKE_SOURCE_DIR}/core/cadabra2 ${CDBOUT}/${NBTEST}.cdb)
  set_tests_properties(${NBTEST}      PROPERTIES ENVIRONMENT "PYTHONPATH=${PYTHONPATH}:$ENV{PYTHONPATH}")
endforeach()

# The main test suite.
#
set(RTESTS
  packages
  dummies
  trigonometric
  noncovariant
  algebra
  components
  factor
  field_theory
  gamma_paper
  fierz
  selecting
  kaluza_klein
  paper
  basic
  numerical
  substitute
  canonicalise
  derivative
  relativity
  spinors
  symmetry
  vary
  young
  )
foreach(RTEST ${RTESTS})
  add_test(${RTEST}  ${CMAKE_SOURCE_DIR}/core/cadabra2 ${CMAKE_SOURCE_DIR}/tests/${RTEST}.cdb)
  # We need to set the Python path so that we pick up the correct cadabra2.so Python module
  # even if we did not do 'make install' yet.
  set_tests_properties(${RTEST}      PROPERTIES ENVIRONMENT "PYTHONPATH=${PYTHONPATH}:$ENV{PYTHONPATH}")
endforeach()

# This does not work properly, and on OSX it fails because we do not
# use any other include settings, unless we hard-code the path to
# where we expect gmpxx to be if Homebrew installed it.
if(APPLE)
   add_definitions("-I/usr/local/include")
endif()
find_library(GMP_LIB gmp REQUIRED)
find_library(GMPXX_LIB gmpxx REQUIRED)

include_directories(${CMAKE_SOURCE_DIR}/core ${GMPXX_LIB_INCLUDE_DIRS})

if (!WIN32)
  add_executable(test_comparison test_comparison.cc)
  target_link_libraries(test_comparison cadabra2)
endif()
