cmake_minimum_required(VERSION 2.8)

project(CadabraTests)

message("-- Configuring self-tests")

add_test(basic   cadabra2 ${CMAKE_SOURCE_DIR}/tests/in.cdb)
