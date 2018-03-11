cmake_minimum_required(VERSION 3.1)
project(Cadabra)
message("------------------------------------------------------------------------\nConfiguring 'client_server'")

#---------------------------------------------------------------------------
# Preamble.
#---------------------------------------------------------------------------

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.9)
    message(FATAL_ERROR "GCC version must be at least 4.9 for regex support! See http://askubuntu.com/questions/428198/getting-installing-gcc-g-4-9-on-ubuntu and then set the environment variables CXX to g++-4.9 and CC to gcc-4.9. You may have to erase the build directory before re-running cmake.")
  endif()
endif()

#---------------------------------------------------------------------------
# Locate libraries.
#---------------------------------------------------------------------------

find_package(GLIBMM)
find_package(SQLite3)
find_package(Threads)
find_package(Boost 1.53.0 COMPONENTS system program_options date_time filesystem REQUIRED)

#---------------------------------------------------------------------------
# Enumerate input files.
#---------------------------------------------------------------------------

set(CADABRA_SERVER_SRC
  Server.cc 
  Snoop.cc 
  jsoncpp/jsoncpp.cpp 
  ../core/CdbPython.cc 
  ../core/Stopwatch.cc 
  ../core/ProgressMonitor.cc
  ../libs/whereami/whereami.c
  ../core/InstallPrefix.cc
#  ../libs/sqlite3/sqlite3.c  FIXME: only on windows
)

set(CADABRA_CLIENT_SRC
  ComputeThread.cc 
  DocumentThread.cc 
  ../core/DataCell.cc 
  Actions.cc 
  Snoop.cc 
  jsoncpp/jsoncpp.cpp
  ../libs/whereami/whereami.c
  ../core/InstallPrefix.cc
#  ../libs/sqlite3/sqlite3.c
)

#---------------------------------------------------------------------------
# Include directories and preprocessor directives.
#---------------------------------------------------------------------------

include_directories(
  "." 
  "../core"
  "../libs/pybind11/include"
  "websocketpp" 
  "jsoncpp"
  "../libs/internal/include"
  "../libs/whereami"
  ${Boost_INCLUDE_DIRS}
  ${PYTHON_INCLUDE_DIRS}
  ${SQLITE3_INCLUDE_DIR}
)

if(CMAKE_COMPILER_IS_GNUCXX)
  add_definitions("-Wall -g -Wno-unused-but-set-variable")
endif()
add_definitions(
  -D_WEBSOCKETPP_CPP11_STL_
  -DBOOST_ASIO_HAS_STD_CHRONO
  -DNOMINMAX
  -DJSON_DLL_BUILD
  )
if(MSVC)
  #  set_target_properties(cadabra2-gtk PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS" LINK_FLAGS "/ENTRY:mainCRTStartup")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
endif()

#---------------------------------------------------------------------------
# Make targets.
#---------------------------------------------------------------------------

# Server library
# add_library(cadabra_server SHARED ${CADABRA_SERVER_SRC} )
# target_link_libraries(cadabra_server
#   ${Boost_LIBRARIES}
#   ${SNOOP_LIBRARIES}
#   ${SQLITE3_LIBRARIES}
#   ${PYTHON_LIBRARIES}
#   ${GLIBMM3_LIBRARIES} 
#   Threads::Threads
#   )

# Server executable
add_executable(cadabra-server cadabra-server.cc ${CADABRA_SERVER_SRC})
# target_link_libraries(cadabra-server cadabra_server)
target_link_libraries(cadabra-server
  pybind11::embed
  ${Boost_LIBRARIES}
  ${SQLITE3_LIBRARIES}
  ${PYTHON_LIBRARIES}
  ${GLIBMM3_LIBRARIES} 
  Threads::Threads
  )
if(WIN32)
   target_link_libraries(cadabra-server ws2_32 mswsock)
endif()

# cadabra2html
add_executable(cadabra2html cadabra2html.cc ../core/DataCell.cc)
target_link_libraries(cadabra2html cadabra_client)

# Client library
add_library(cadabra_client STATIC ${CADABRA_CLIENT_SRC})
target_link_libraries(cadabra_client
  ${Boost_LIBRARIES}
  ${SQLITE3_LIBRARIES}
  ${PYTHON_LIBRARIES}
  ${GLIBMM3_LIBRARIES}
  Threads::Threads
  )
if(WIN32)
   target_link_libraries(cadabra_client ws2_32)
endif()


#---------------------------------------------------------------------------
# Installation.
#---------------------------------------------------------------------------

# The preamble 'notebook.tex' should be kept in sync with the one in
# ../frontend/common/texengine; this one is meant for printing so
# slightly different, but cadabra-specific macros should be copied.
set(INSTALL_LATEX_DIR "${CMAKE_INSTALL_PREFIX}/share/cadabra2")
install(FILES  notebook.tex  DESTINATION ${INSTALL_LATEX_DIR})
install(FILES  notebook.html DESTINATION ${INSTALL_LATEX_DIR})

#set(INSTALL_LIB_DIR     lib           CACHE PATH "Installation directory for libraries")
install(TARGETS cadabra-server DESTINATION bin)
install(TARGETS cadabra2html DESTINATION bin)
install(TARGETS cadabra_client RUNTIME DESTINATION bin COMPONENT runtime LIBRARY ARCHIVE DESTINATION lib LIBRARY DESTINATION lib)
# install(TARGETS cadabra_server LIBRARY ARCHIVE DESTINATION lib LIBRARY DESTINATION lib)

# Install shared libraries and gtk helpers on Windows.
if(WIN32)
  install(DIRECTORY "${PROJECT_BINARY_DIR}/${CADABRA_BUILD_TYPE}/" DESTINATION bin FILES_MATCHING PATTERN "*.dll")  
  install(
	 FILES
		"${_VCPKG_ROOT_DIR}/installed/x64-windows/tools/glib/gspawn-win64-helper.exe"		
		"${_VCPKG_ROOT_DIR}/installed/x64-windows/tools/glib/gspawn-win64-helper-console.exe"
	 DESTINATION
	   bin)
endif()


