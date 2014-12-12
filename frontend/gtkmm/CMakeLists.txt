cmake_minimum_required(VERSION 2.8)

project(CadabraGtkmm)

if(CMAKE_COMPILER_IS_GNUCXX)
  add_definitions("-Wall -g -std=c++11 -Wno-unused-but-set-variable -D_WEBSOCKETPP_CPP11_STL_")
endif()

# Gtkmm-3.0 dependencies. 
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTKMM3 REQUIRED gtkmm-3.0)
include_directories(${GTKMM3_INCLUDE_DIRS})
link_directories(${GTKMM3_LIBRARY_DIRS})
add_definitions(${GTKMM3_CFLAGS_OTHER})

include_directories("." "../../client_server")

add_executable(cadabra-gtk main.cc 
  NotebookWindow.cc 
  NotebookCanvas.cc 
  TeXView.cc 
  CodeInput.cc 
  ../common/TeXEngine.cc)

target_link_libraries(cadabra-gtk cadabra_client texengine ${Boost_LIBRARIES} ${GTKMM3_LIBRARIES} -lpthread )

install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/cadabra-gtk DESTINATION bin)
