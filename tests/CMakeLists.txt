cmake_minimum_required(VERSION 2.8)

project(CadabraTests)

message("-- Configuring self-tests")

add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure)

add_test(basic          cadabra2 ${CMAKE_SOURCE_DIR}/tests/in.cdb)
add_test(substitute     cadabra2 ${CMAKE_SOURCE_DIR}/tests/substitute.cdb)
add_test(canonicalise   cadabra2 ${CMAKE_SOURCE_DIR}/tests/canonicalise.cdb)
