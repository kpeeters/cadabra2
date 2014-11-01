cmake_minimum_required(VERSION 2.8)

project(Cadabra)

add_executable(cadabra-osx fake.cc)
add_custom_command(TARGET cadabra-osx
		   POST_BUILD
                   COMMAND xcodebuild 
		   COMMAND rm ${CMAKE_CURRENT_BINARY_DIR}/cadabra-osx
		   WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Cadabra
		   )
