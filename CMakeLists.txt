cmake_minimum_required(VERSION 2.8)

project(Cadabra)

# Set path to additional cmake files
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules")

subdirs(core client_server frontend)
