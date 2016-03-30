cmake_minimum_required(VERSION 2.8)

project(CadabraTests)

message("-- Configuring self-tests")

add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure)

set(ENV{PYTHONPATH} ${CMAKE_BINARY_DIR}/core)
set(CTEST_ENVIRONMENT "PYTHONPATH = ${CMAKE_BINARY_DIR}/core")
message("-- Set PYTHONPATH for testing to $ENV{PYTHONPATH}")

add_test(comparison     test_comparison)
add_test(field_theory   ${CMAKE_SOURCE_DIR}/core/cadabra2 ${CMAKE_SOURCE_DIR}/tests/field_theory.cdb)
add_test(selecting      ${CMAKE_SOURCE_DIR}/core/cadabra2 ${CMAKE_SOURCE_DIR}/tests/selecting.cdb)
add_test(paper          ${CMAKE_SOURCE_DIR}/core/cadabra2 ${CMAKE_SOURCE_DIR}/tests/paper.cdb)
add_test(basic          ${CMAKE_SOURCE_DIR}/core/cadabra2 ${CMAKE_SOURCE_DIR}/tests/basic.cdb)
add_test(substitute     ${CMAKE_SOURCE_DIR}/core/cadabra2 ${CMAKE_SOURCE_DIR}/tests/substitute.cdb)
add_test(canonicalise   ${CMAKE_SOURCE_DIR}/core/cadabra2 ${CMAKE_SOURCE_DIR}/tests/canonicalise.cdb)
add_test(derivative     ${CMAKE_SOURCE_DIR}/core/cadabra2 ${CMAKE_SOURCE_DIR}/tests/derivative.cdb)
add_test(symmetry       ${CMAKE_SOURCE_DIR}/core/cadabra2 ${CMAKE_SOURCE_DIR}/tests/symmetry.cdb)
add_test(young          ${CMAKE_SOURCE_DIR}/core/cadabra2 ${CMAKE_SOURCE_DIR}/tests/young.cdb)

include_directories(${CMAKE_SOURCE_DIR}/core)
add_executable(test_comparison test_comparison.cc)
target_link_libraries(test_comparison cadabra2)
