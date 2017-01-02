cmake_minimum_required(VERSION 2.8)

project(CadabraTests)

message("-- Configuring self-tests")

add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure)

set(PYTHONPATH ${CMAKE_SOURCE_DIR}/core:${CMAKE_SOURCE_DIR}/core/packages:${CMAKE_BINARY_DIR}/core)
message("-- Set PYTHONPATH for testing to ${PYTHONPATH}:$ENV{PYTHONPATH}")

if (!WIN32)
  add_test(comparison     test_comparison)
endif()


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
  add_custom_command(OUTPUT  ${CDBOUT}/${NBTEST}.cdb
	 DEPENDS ${CNBIN}/${NBTEST}.cnb ${DC}
	 COMMAND cadabra2cadabra
	 ARGS    ${CNBIN}/${NBTEST}.cnb ${CDBOUT}/${NBTEST}.cdb
	 COMMENT "Creating ${NBTEST} notebook...")
  add_custom_target("${NBTEST}_test" ALL echo -n DEPENDS ${CDBOUT}/${NBTEST}.cdb)
  add_test(${NBTEST}  ${CMAKE_SOURCE_DIR}/core/cadabra2 ${CDBOUT}/${NBTEST}.cdb)
endforeach()

# The main test suite.
#
add_test(packages       ${CMAKE_SOURCE_DIR}/core/cadabra2 ${CMAKE_SOURCE_DIR}/tests/packages.cdb)
add_test(algebra        ${CMAKE_SOURCE_DIR}/core/cadabra2 ${CMAKE_SOURCE_DIR}/tests/algebra.cdb)
add_test(components     ${CMAKE_SOURCE_DIR}/core/cadabra2 ${CMAKE_SOURCE_DIR}/tests/components.cdb)
add_test(factor         ${CMAKE_SOURCE_DIR}/core/cadabra2 ${CMAKE_SOURCE_DIR}/tests/factor.cdb)
add_test(field_theory   ${CMAKE_SOURCE_DIR}/core/cadabra2 ${CMAKE_SOURCE_DIR}/tests/field_theory.cdb)
add_test(gamma_paper    ${CMAKE_SOURCE_DIR}/core/cadabra2 ${CMAKE_SOURCE_DIR}/tests/gamma_paper.cdb)
add_test(fierz          ${CMAKE_SOURCE_DIR}/core/cadabra2 ${CMAKE_SOURCE_DIR}/tests/fierz.cdb)
add_test(selecting      ${CMAKE_SOURCE_DIR}/core/cadabra2 ${CMAKE_SOURCE_DIR}/tests/selecting.cdb)
add_test(kaluza_klein   ${CMAKE_SOURCE_DIR}/core/cadabra2 ${CMAKE_SOURCE_DIR}/tests/kaluza_klein.cdb)
add_test(paper          ${CMAKE_SOURCE_DIR}/core/cadabra2 ${CMAKE_SOURCE_DIR}/tests/paper.cdb)
add_test(basic          ${CMAKE_SOURCE_DIR}/core/cadabra2 ${CMAKE_SOURCE_DIR}/tests/basic.cdb)
add_test(numerical      ${CMAKE_SOURCE_DIR}/core/cadabra2 ${CMAKE_SOURCE_DIR}/tests/numerical.cdb)
add_test(substitute     ${CMAKE_SOURCE_DIR}/core/cadabra2 ${CMAKE_SOURCE_DIR}/tests/substitute.cdb)
add_test(canonicalise   ${CMAKE_SOURCE_DIR}/core/cadabra2 ${CMAKE_SOURCE_DIR}/tests/canonicalise.cdb)
add_test(derivative     ${CMAKE_SOURCE_DIR}/core/cadabra2 ${CMAKE_SOURCE_DIR}/tests/derivative.cdb)
add_test(relativity     ${CMAKE_SOURCE_DIR}/core/cadabra2 ${CMAKE_SOURCE_DIR}/tests/relativity.cdb)
add_test(symmetry       ${CMAKE_SOURCE_DIR}/core/cadabra2 ${CMAKE_SOURCE_DIR}/tests/symmetry.cdb)
add_test(vary           ${CMAKE_SOURCE_DIR}/core/cadabra2 ${CMAKE_SOURCE_DIR}/tests/vary.cdb)
add_test(young          ${CMAKE_SOURCE_DIR}/core/cadabra2 ${CMAKE_SOURCE_DIR}/tests/young.cdb)

# We need to set the Python path so that we pick up the correct cadabra2.so Python module
# even if we did not do 'make install' yet.
set_tests_properties(algebra      PROPERTIES ENVIRONMENT "PYTHONPATH=${PYTHONPATH}:$ENV{PYTHONPATH}")
set_tests_properties(components   PROPERTIES ENVIRONMENT "PYTHONPATH=${PYTHONPATH}:$ENV{PYTHONPATH}")
set_tests_properties(packages     PROPERTIES ENVIRONMENT "PYTHONPATH=${PYTHONPATH}:$ENV{PYTHONPATH}")
set_tests_properties(factor       PROPERTIES ENVIRONMENT "PYTHONPATH=${PYTHONPATH}:$ENV{PYTHONPATH}")
set_tests_properties(field_theory PROPERTIES ENVIRONMENT "PYTHONPATH=${PYTHONPATH}:$ENV{PYTHONPATH}")
set_tests_properties(gamma_paper  PROPERTIES ENVIRONMENT "PYTHONPATH=${PYTHONPATH}:$ENV{PYTHONPATH}")
set_tests_properties(fierz        PROPERTIES ENVIRONMENT "PYTHONPATH=${PYTHONPATH}:$ENV{PYTHONPATH}")
set_tests_properties(selecting    PROPERTIES ENVIRONMENT "PYTHONPATH=${PYTHONPATH}:$ENV{PYTHONPATH}")
set_tests_properties(kaluza_klein PROPERTIES ENVIRONMENT "PYTHONPATH=${PYTHONPATH}:$ENV{PYTHONPATH}")
set_tests_properties(paper        PROPERTIES ENVIRONMENT "PYTHONPATH=${PYTHONPATH}:$ENV{PYTHONPATH}")
set_tests_properties(basic        PROPERTIES ENVIRONMENT "PYTHONPATH=${PYTHONPATH}:$ENV{PYTHONPATH}")
set_tests_properties(numerical    PROPERTIES ENVIRONMENT "PYTHONPATH=${PYTHONPATH}:$ENV{PYTHONPATH}")
set_tests_properties(substitute   PROPERTIES ENVIRONMENT "PYTHONPATH=${PYTHONPATH}:$ENV{PYTHONPATH}")
set_tests_properties(canonicalise PROPERTIES ENVIRONMENT "PYTHONPATH=${PYTHONPATH}:$ENV{PYTHONPATH}")
set_tests_properties(derivative   PROPERTIES ENVIRONMENT "PYTHONPATH=${PYTHONPATH}:$ENV{PYTHONPATH}")
set_tests_properties(relativity   PROPERTIES ENVIRONMENT "PYTHONPATH=${PYTHONPATH}:$ENV{PYTHONPATH}")
set_tests_properties(symmetry     PROPERTIES ENVIRONMENT "PYTHONPATH=${PYTHONPATH}:$ENV{PYTHONPATH}")
set_tests_properties(vary         PROPERTIES ENVIRONMENT "PYTHONPATH=${PYTHONPATH}:$ENV{PYTHONPATH}")
set_tests_properties(young        PROPERTIES ENVIRONMENT "PYTHONPATH=${PYTHONPATH}:$ENV{PYTHONPATH}")

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
