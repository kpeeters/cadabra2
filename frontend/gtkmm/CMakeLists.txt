cmake_minimum_required(VERSION 2.8)

project(CadabraGtkmm)

if(CMAKE_COMPILER_IS_GNUCXX)
  add_definitions("-Wall -g -std=c++11 -Wno-unused-but-set-variable -D_WEBSOCKETPP_CPP11_STL_")
endif()
add_definitions("-DBOOST_LOG_DYN_LINK")

# Gtkmm-3.0 dependencies. 
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTKMM3 REQUIRED gtkmm-3.0)
include_directories(${GTKMM3_INCLUDE_DIRS})
link_directories(${GTKMM3_LIBRARY_DIRS})
add_definitions(${GTKMM3_CFLAGS_OTHER})
find_package(Boost 1.45.0 COMPONENTS system)
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
include_directories("${Boost_INCLUDE_DIRS}")
pkg_check_modules(UUID uuid REQUIRED)
link_directories(${UUID_LIBRARY_DIRS})

include_directories("." "../../client_server" "../../client_server/jsoncpp")

add_executable(cadabra-gtk main.cc 
  NotebookWindow.cc 
  NotebookCanvas.cc 
  TeXView.cc 
  CodeInput.cc 
  ImageView.cc
  Cadabra.cc
  ../common/TeXEngine.cc)

target_link_libraries(cadabra-gtk cadabra_client texengine ${Boost_LIBRARIES} ${GTKMM3_LIBRARIES} -lpthread ${UUID_LIBRARIES})

# Create installation directories with correct permissions.
set(INSTALL_SHARE_DIR     share/cadabra2         CACHE PATH "Installation directory for shared files.")
set(INSTALL_IMAGES_DIR       share/cadabra2/images  CACHE PATH "Installation directory for image files.")
install(DIRECTORY DESTINATION ${INSTALL_SHARE_DIR} DIRECTORY_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE 
                                                             GROUP_READ GROUP_EXECUTE 
                                                             WORLD_READ WORLD_EXECUTE)
install(DIRECTORY DESTINATION ${INSTALL_IMAGES_DIR} DIRECTORY_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE 
                                                             GROUP_READ GROUP_EXECUTE 
                                                             WORLD_READ WORLD_EXECUTE)

# Install programs.
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/cadabra-gtk     DESTINATION bin)
install(FILES    ${PROJECT_SOURCE_DIR}/../../images/cadabra.png DESTINATION ${INSTALL_IMAGES_DIR})

