cmake_minimum_required(VERSION 2.8)

project(CadabraGtkmm)

if(CMAKE_COMPILER_IS_GNUCXX)
  add_definitions("-Wall -g -std=c++11 -Wno-unused-but-set-variable -D_WEBSOCKETPP_CPP11_STL_")
endif()

include_directories("." "../../client_server")
add_executable(cadabra-gtk main.cc)

target_link_libraries(cadabra-gtk cadabra_client ${Boost_LIBRARIES} -lpthread)
