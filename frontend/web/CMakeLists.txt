
# Cadabra for the web.

cmake_minimum_required(VERSION 3.5)

project(CadabraWeb)

set(CMAKE_CXX_STANDARD 14)

if(NOT "${CMAKE_SYSTEM_NAME}" STREQUAL "Emscripten")
  message(FATAL_ERROR
	 "Emscripten toolchain not active, "
    "system name is ${CMAKE_SYSTEM_NAME}, "
	 "did you 'source ${EMSDK}/emsdk_env.sh' "
	 "and use the correct CMAKE_TOOLCHAIN_FILE:"
    "-DCMAKE_TOOLCHAIN_FILE=${EMSCRIPTEN}/cmake/Modules/Platform/Emscripten.cmake"
	 )
else()
  message(STATUS "Emscripten active, good.")
endif()


#----------------------------------------------------------------------------------------------
# Building the native (C++) part of the app.
#----------------------------------------------------------------------------------------------

set(SOURCES
  ../../core/DataCell.cc
  ../../client_server/DocumentThread.cc
  src/NotebookWindow.cc
  )

include_directories(
  ../../libs/jsoncpp
  ../../libs/internal/include
  ../../core
  ../../client_server
  )

add_executable(cadabra ${SOURCES})
