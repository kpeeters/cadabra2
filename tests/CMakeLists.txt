cmake_minimum_required(VERSION 2.8)

project(CadabraTests)

message("-- Configuring self-tests")

add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure)

set(PYTHONPATH ${CMAKE_BINARY_DIR}/core)
message("-- Set PYTHONPATH for testing to ${PYTHONPATH}:$ENV{PYTHONPATH}")

if(APPLE)
  message("-- Not running comparison test on OS X")
else()
  add_test(comparison     test_comparison)
endif()
add_test(components     ${CMAKE_SOURCE_DIR}/core/cadabra2 ${CMAKE_SOURCE_DIR}/tests/components.cdb)
add_test(factor         ${CMAKE_SOURCE_DIR}/core/cadabra2 ${CMAKE_SOURCE_DIR}/tests/factor.cdb)
add_test(field_theory   ${CMAKE_SOURCE_DIR}/core/cadabra2 ${CMAKE_SOURCE_DIR}/tests/field_theory.cdb)
add_test(gamma_paper    ${CMAKE_SOURCE_DIR}/core/cadabra2 ${CMAKE_SOURCE_DIR}/tests/gamma_paper.cdb)
add_test(selecting      ${CMAKE_SOURCE_DIR}/core/cadabra2 ${CMAKE_SOURCE_DIR}/tests/selecting.cdb)
add_test(kaluza_klein   ${CMAKE_SOURCE_DIR}/core/cadabra2 ${CMAKE_SOURCE_DIR}/tests/kaluza_klein.cdb)
add_test(paper          ${CMAKE_SOURCE_DIR}/core/cadabra2 ${CMAKE_SOURCE_DIR}/tests/paper.cdb)
add_test(basic          ${CMAKE_SOURCE_DIR}/core/cadabra2 ${CMAKE_SOURCE_DIR}/tests/basic.cdb)
add_test(substitute     ${CMAKE_SOURCE_DIR}/core/cadabra2 ${CMAKE_SOURCE_DIR}/tests/substitute.cdb)
add_test(canonicalise   ${CMAKE_SOURCE_DIR}/core/cadabra2 ${CMAKE_SOURCE_DIR}/tests/canonicalise.cdb)
add_test(derivative     ${CMAKE_SOURCE_DIR}/core/cadabra2 ${CMAKE_SOURCE_DIR}/tests/derivative.cdb)
add_test(symmetry       ${CMAKE_SOURCE_DIR}/core/cadabra2 ${CMAKE_SOURCE_DIR}/tests/symmetry.cdb)
add_test(vary           ${CMAKE_SOURCE_DIR}/core/cadabra2 ${CMAKE_SOURCE_DIR}/tests/vary.cdb)
add_test(young          ${CMAKE_SOURCE_DIR}/core/cadabra2 ${CMAKE_SOURCE_DIR}/tests/young.cdb)


# We need to set the Python path so that we pick up the correct cadabra2.so Python module
# even if we did not do 'make install' yet.
set_tests_properties(components   PROPERTIES ENVIRONMENT "PYTHONPATH=${PYTHONPATH}:$ENV{PYTHONPATH}")
set_tests_properties(factor       PROPERTIES ENVIRONMENT "PYTHONPATH=${PYTHONPATH}:$ENV{PYTHONPATH}")
set_tests_properties(field_theory PROPERTIES ENVIRONMENT "PYTHONPATH=${PYTHONPATH}:$ENV{PYTHONPATH}")
set_tests_properties(gamma_paper  PROPERTIES ENVIRONMENT "PYTHONPATH=${PYTHONPATH}:$ENV{PYTHONPATH}")
set_tests_properties(selecting    PROPERTIES ENVIRONMENT "PYTHONPATH=${PYTHONPATH}:$ENV{PYTHONPATH}")
set_tests_properties(kaluza_klein PROPERTIES ENVIRONMENT "PYTHONPATH=${PYTHONPATH}:$ENV{PYTHONPATH}")
set_tests_properties(paper        PROPERTIES ENVIRONMENT "PYTHONPATH=${PYTHONPATH}:$ENV{PYTHONPATH}")
set_tests_properties(basic        PROPERTIES ENVIRONMENT "PYTHONPATH=${PYTHONPATH}:$ENV{PYTHONPATH}")
set_tests_properties(substitute   PROPERTIES ENVIRONMENT "PYTHONPATH=${PYTHONPATH}:$ENV{PYTHONPATH}")
set_tests_properties(canonicalise PROPERTIES ENVIRONMENT "PYTHONPATH=${PYTHONPATH}:$ENV{PYTHONPATH}")
set_tests_properties(derivative   PROPERTIES ENVIRONMENT "PYTHONPATH=${PYTHONPATH}:$ENV{PYTHONPATH}")
set_tests_properties(symmetry     PROPERTIES ENVIRONMENT "PYTHONPATH=${PYTHONPATH}:$ENV{PYTHONPATH}")
set_tests_properties(vary         PROPERTIES ENVIRONMENT "PYTHONPATH=${PYTHONPATH}:$ENV{PYTHONPATH}")
set_tests_properties(young        PROPERTIES ENVIRONMENT "PYTHONPATH=${PYTHONPATH}:$ENV{PYTHONPATH}")

# This does not work properly, and on OSX it fails because we do not use any other include
# settings.
find_library(GMP_LIB gmp REQUIRED)
find_library(GMPXX_LIB gmpxx REQUIRED)

include_directories(${CMAKE_SOURCE_DIR}/core ${GMPXX_LIB_INCLUDE_DIRS})
add_executable(test_comparison test_comparison.cc)
target_link_libraries(test_comparison cadabra2)
