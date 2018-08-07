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


#---------------------------------------------------------------------------
# Enumerate input files.
#---------------------------------------------------------------------------

set(CADABRA_SERVER_SRC
	cadabra-server.cc
	Server.cc 
	Snoop.cc 
	${CADABRA_CORE_DIR}/InstallPrefix.cc
	${CADABRA_CORE_DIR}/CdbPython.cc 
	${CADABRA_CORE_DIR}/Stopwatch.cc 
	${CADABRA_CORE_DIR}/ProgressMonitor.cc
	${CADABRA_LIBS_DIR}/jsoncpp/jsoncpp.cpp 
	${CADABRA_LIBS_DIR}/whereami/whereami.c
)

set(CADABRA_CLIENT_SRC
	ComputeThread.cc 
	DocumentThread.cc 
	Actions.cc 
	Snoop.cc 
	${CADABRA_CORE_DIR}/DataCell.cc 
	${CADABRA_CORE_DIR}/InstallPrefix.cc
	${CADABRA_LIBS_DIR}/jsoncpp/jsoncpp.cpp
	${CADABRA_LIBS_DIR}/whereami/whereami.c
)

#---------------------------------------------------------------------------
# Include directories and preprocessor directives.
#---------------------------------------------------------------------------

include_directories(
	"." 
	"${CADABRA_CORE_DIR}"
	"${CADABRA_LIBS_DIR}/pybind11/include"
	"${CADABRA_LIBS_DIR}/websocketpp" 
	"${CADABRA_LIBS_DIR}/jsoncpp"
	"${CADABRA_LIBS_DIR}/internal/include"
	"${CADABRA_LIBS_DIR}/whereami"
	${Boost_INCLUDE_DIRS}
	${PYTHON_INCLUDE_DIRS}
	${SQLITE3_INCLUDE_DIR}
)

add_definitions(
	-D_WEBSOCKETPP_CPP11_STL_
	-DBOOST_ASIO_HAS_STD_CHRONO
  )

#---------------------------------------------------------------------------
# Make targets.
#---------------------------------------------------------------------------

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
if(WIN32)
   target_link_libraries(cadabra-server ws2_32 mswsock bcrypt)
endif()

# cadabra2html
add_executable(cadabra2html 
	cadabra2html.cc 
	${CADABRA_CORE_DIR}/DataCell.cc
)
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
   target_link_libraries(cadabra_client ws2_32 bcrypt)
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
install(TARGETS cadabra2html DESTINATION bin)
install(
	TARGETS cadabra_client 
	RUNTIME DESTINATION bin COMPONENT runtime 
	LIBRARY ARCHIVE DESTINATION lib 
	LIBRARY DESTINATION lib
)
# install(TARGETS cadabra_server LIBRARY ARCHIVE DESTINATION lib LIBRARY DESTINATION lib)

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
	message("--- Python library:   ${CDB_PYTHON_BASE}.dll")
	string(TOLOWER ${CDB_PYTHON_BASE} CDB_PYTHON_BASE)
   install(FILES "${CDB_PYTHON_PATH}/${CDB_PYTHON_BASE}.dll"
     DESTINATION    bin
   )
endif()
