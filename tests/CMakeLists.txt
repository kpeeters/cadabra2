cmake_minimum_required(VERSION 2.8)

project(CadabraTests)

message("-- Configuring self-tests")

add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure)

add_test(comparison     test_comparison)
add_test(field_theory   cadabra2 ${CMAKE_SOURCE_DIR}/tests/field_theory.cdb)
add_test(paper          cadabra2 ${CMAKE_SOURCE_DIR}/tests/paper.cdb)
add_test(basic          cadabra2 ${CMAKE_SOURCE_DIR}/tests/basic.cdb)
add_test(substitute     cadabra2 ${CMAKE_SOURCE_DIR}/tests/substitute.cdb)
add_test(canonicalise   cadabra2 ${CMAKE_SOURCE_DIR}/tests/canonicalise.cdb)
add_test(derivative     cadabra2 ${CMAKE_SOURCE_DIR}/tests/derivative.cdb)
add_test(symmetry       cadabra2 ${CMAKE_SOURCE_DIR}/tests/symmetry.cdb)
add_test(young          cadabra2 ${CMAKE_SOURCE_DIR}/tests/young.cdb)

include_directories(${CMAKE_SOURCE_DIR}/core)
add_executable(test_comparison test_comparison.cc)
target_link_libraries(test_comparison cadabra2)
