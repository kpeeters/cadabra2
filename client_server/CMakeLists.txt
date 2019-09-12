cmake_minimum_required(VERSION ${CADABRA_CMAKE_VERSION})
project(Cadabra)


#---------------------------------------------------------------------------
# Preamble.
#---------------------------------------------------------------------------

print_header("Configuring client-server")

set(INSTALL_LATEX_DIR "${CMAKE_INSTALL_PREFIX}/share/cadabra2")


#---------------------------------------------------------------------------
# Locate libraries.
#---------------------------------------------------------------------------

find_package(GLIBMM  REQUIRED)
find_package(SQLite3 REQUIRED)
find_package(Threads REQUIRED)
find_package(Boost 1.53.0 COMPONENTS system program_options date_time filesystem REQUIRED)
if(ENABLE_SYSTEM_JSONCPP)
  message("-- Linking against system-provided jsoncpp")
  find_package(JSONCPP REQUIRED)
else()
  message("-- Linking against included jsoncpp")
endif()

#---------------------------------------------------------------------------
# Enumerate input files.
#---------------------------------------------------------------------------

set(CADABRA_SERVER_SRC
	cadabra-server.cc
	Server.cc 
	${CADABRA_CORE_DIR}/InstallPrefix.cc
	${CADABRA_CORE_DIR}/CdbPython.cc 
	${CADABRA_CORE_DIR}/Stopwatch.cc 
	${CADABRA_LIBS_DIR}/whereami/whereami.c
	)
if(NOT CONDA_FOUND)
  set(CADABRA_SERVER_SRC
	 ${CADABRA_SERVER_SRC}
	 Snoop.cc
	 )
endif()

set(CADABRA_CLIENT_SRC
	ComputeThread.cc 
	DocumentThread.cc 
	Actions.cc 
	Snoop.cc 
	${CADABRA_CORE_DIR}/DataCell.cc 
	${CADABRA_CORE_DIR}/InstallPrefix.cc
	${CADABRA_LIBS_DIR}/whereami/whereami.c
	)

if(ENABLE_SYSTEM_JSONCPP)
else()
  list(APPEND CADABRA_SERVER_SRC
	 ${CADABRA_LIBS_DIR}/jsoncpp/jsoncpp.cpp
	 )
  list(APPEND CADABRA_CLIENT_SRC
	 ${CADABRA_LIBS_DIR}/jsoncpp/jsoncpp.cpp
	 )
endif() 

set(JUPYTER_KERNEL_SRC
    cadabra-jupyter-kernel.cc
    cadabra-jupyter-kernel.hh
    cadabra-jupyter-main.cc
	 Server.cc
	 ${CADABRA_CORE_DIR}/InstallPrefix.cc
	 ${CADABRA_CORE_DIR}/CdbPython.cc 
	 ${CADABRA_CORE_DIR}/Stopwatch.cc 
	 ${CADABRA_LIBS_DIR}/jsoncpp/jsoncpp.cpp 
	 ${CADABRA_LIBS_DIR}/whereami/whereami.c
	 )
  

# set(CONNECTION_FILE ${CMAKE_CURRENT_SOURCE_DIR}/connection.json)
# 
# add_custom_command(TARGET echo_kernel POST_BUILD
#                    COMMAND ${CMAKE_COMMAND} -E
#                    copy "${CONNECTION_FILE}" "${CMAKE_CURRENT_BINARY_DIR}/")
# 
# add_custom_command(TARGET echo_kernel POST_BUILD
#                    COMMAND ${CMAKE_COMMAND} -E
# 						 copy "${CMAKE_BINARY_DIR}/${XEUS_RUNTIME_FILE}" "${CMAKE_CURRENT_BINARY_DIR}/")

#---------------------------------------------------------------------------
# Include directories and preprocessor directives.
#---------------------------------------------------------------------------

include_directories(
	"." 
	"${CADABRA_CORE_DIR}"
	"${CADABRA_LIBS_DIR}/pybind11/include"
	"${CADABRA_LIBS_DIR}/websocketpp" 
	"${CADABRA_LIBS_DIR}/internal/include"
	"${CADABRA_LIBS_DIR}/whereami"
	${Boost_INCLUDE_DIRS}
	${PYTHON_INCLUDE_DIRS}
	${SQLITE3_INCLUDE_DIR}
)
if(ENABLE_SYSTEM_JSONCPP)
  include_directories(
	 ${JSONCPP_INCLUDE_DIRS}
	 )
else()
  include_directories(
	 "${CADABRA_LIBS_DIR}/jsoncpp"
	 )
endif()

add_definitions(
	-D_WEBSOCKETPP_CPP11_STL_
	-DBOOST_ASIO_HAS_STD_CHRONO
  )

#---------------------------------------------------------------------------
# Make targets.
#---------------------------------------------------------------------------

# add_executable(tst tst.cc)
# target_link_libraries(tst
# 	${Boost_LIBRARIES}
# 	Threads::Threads
# 	)

# Server executable
if(MSVC AND CADABRA_DEBUG_BUILD)
	add_executable(cadabra-server ${CADABRA_SERVER_SRC})
else()
	add_executable(cadabra-server WIN32 ${CADABRA_SERVER_SRC}) 
endif()

# target_link_libraries(cadabra-server cadabra_server)
target_link_libraries(cadabra-server
	pybind11::embed
	${Boost_LIBRARIES}
	${SQLITE3_LIBRARIES}
	${PYTHON_LIBRARIES}
	${GLIBMM3_LIBRARIES} 
	Threads::Threads
	)
if(ENABLE_SYSTEM_JSONCPP)
  target_link_libraries(cadabra-server
	 ${JSONCPP_LIBRARIES}
	 )
endif()
if(WIN32)
   target_link_libraries(cadabra-server ws2_32 mswsock bcrypt)
endif()

# cadabra2html
if(NOT ENABLE_JUPYTER)
  add_executable(cadabra2html 
	 cadabra2html.cc 
	 )
  target_link_libraries(cadabra2html cadabra_client)
endif()

# cadabra2latex
if(NOT ENABLE_JUPYTER)
  add_executable(cadabra2latex 
	 cadabra2latex.cc 
	 )
  target_link_libraries(cadabra2latex cadabra_client)
endif()

# Client library
if(NOT ENABLE_JUPYTER)
  add_library(cadabra_client STATIC ${CADABRA_CLIENT_SRC})
  target_link_libraries(cadabra_client
	 ${Boost_LIBRARIES}
	 ${SQLITE3_LIBRARIES}
	 ${PYTHON_LIBRARIES}
	 ${GLIBMM3_LIBRARIES}
	 Threads::Threads
	 )
  if(WIN32)
    target_link_libraries(cadabra_client ws2_32 bcrypt)
  endif()
endif()

# Jupyter kernel
if(ENABLE_JUPYTER)
  print_header("Configuring Jupyter kernel build")
  find_package(xtl REQUIRED)
  find_package(nlohmann_json REQUIRED)
  find_library(UUID_LIBRARIES uuid REQUIRED)
  include_directories(${CMAKE_INSTALL_PREFIX}/include ${XTL_INCLUDE_DIR} ${NLOHMANN_JSON_INCLUDE_DIR})
  link_directories(${CMAKE_INSTALL_PREFIX}/lib)
  add_executable(cadabra-jupyter-kernel ${JUPYTER_KERNEL_SRC})
  message("-- UUID library at ${UUID_LIBRARIES}")
  target_link_libraries(cadabra-jupyter-kernel xeus Threads::Threads uuid ${Boost_LIBRARIES} ${PYTHON_LIBRARIES})
else()
  print_header("Not building Jupyter kernel")
endif()

# Make sure sqlite3.dll is copied into the build directory on Windows.
if (WIN32)
	add_custom_command(TARGET cadabra_client POST_BUILD 
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
		${SQLITE3_LIBRARIES}
		${PROJECT_BINARY_DIR}/${CADABRA_BUILD_TYPE}/sqlite3.dll
	)
endif()


#---------------------------------------------------------------------------
# Installation.
#---------------------------------------------------------------------------

# The preamble 'notebook.tex' should be kept in sync with the one in
# ../frontend/common/texengine; this one is meant for printing so
# slightly different, but cadabra-specific macros should be copied.

if (NOT INSTALL_TARGETS_ONLY)
	install(FILES  notebook.tex  DESTINATION ${INSTALL_LATEX_DIR})
	install(FILES  notebook.html DESTINATION ${INSTALL_LATEX_DIR})
endif()
#set(INSTALL_LIB_DIR     lib           CACHE PATH "Installation directory for libraries")

install(TARGETS cadabra-server DESTINATION bin)
if(NOT ENABLE_JUPYTER)
  install(TARGETS cadabra2html cadabra2latex DESTINATION bin)
endif()
# We don't need to install the libcadabra_client library as it is statically
# linked with our binaries anyway.
# install(
# 	TARGETS cadabra_client 
# 	RUNTIME DESTINATION bin COMPONENT runtime 
# 	LIBRARY ARCHIVE DESTINATION lib 
# 	LIBRARY DESTINATION lib
# )
# install(TARGETS cadabra_server LIBRARY ARCHIVE DESTINATION lib LIBRARY DESTINATION lib)

if(ENABLE_JUPYTER)
  install(TARGETS cadabra-jupyter-kernel      DESTINATION bin)
  install(FILES   kernel.json connection.json DESTINATION share/jupyter/kernels/cadabra)
endif()

if (MSVC AND NOT INSTALL_TARGETS_ONLY)
	install_dlls_from("client_server")
	# GTK helpers
	install(
		FILES
			"${_VCPKG_ROOT_DIR}/installed/x64-windows/tools/glib/gspawn-win64-helper.exe"		
			"${_VCPKG_ROOT_DIR}/installed/x64-windows/tools/glib/gspawn-win64-helper-console.exe"
		DESTINATION
			bin
			)
	# Python (for some reason this is not copied in)
   get_filename_component(CDB_PYTHON_BASE ${PYTHON_LIBRARIES} NAME_WE)
   get_filename_component(CDB_PYTHON_PATH ${PYTHON_LIBRARIES} DIRECTORY)
	message("--- Python directory: ${CDB_PYTHON_PATH}")
	string(TOLOWER ${CDB_PYTHON_BASE} CDB_PYTHON_BASE)
	message("--- Python library:   ${CDB_PYTHON_BASE}.dll")
   install(FILES "${CDB_PYTHON_PATH}/../${CDB_PYTHON_BASE}.dll"
     DESTINATION    bin
   )
endif()

# manual pages
if(NOT MSVC)
  install(
	 FILES
	 ../man/man1/cadabra-server.1
	 DESTINATION
	 man/man1
	 )
endif()
