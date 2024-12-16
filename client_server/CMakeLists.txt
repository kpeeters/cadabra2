# cmake_minimum_required(VERSION ${CADABRA_CMAKE_VERSION})
project(Cadabra)

if(POLICY CMP0167)
  cmake_policy(SET CMP0167 NEW)
endif()

#---------------------------------------------------------------------------
# Preamble.
#---------------------------------------------------------------------------

print_header("Configuring client-server")

set(INSTALL_LATEX_DIR "share/cadabra2")


#---------------------------------------------------------------------------
# Locate libraries.
#---------------------------------------------------------------------------

find_package(GLIBMM  REQUIRED)
find_package(SQLITE3 REQUIRED)
# Don't set pthreads to required. Either we're on a platform where explict
# linking with -lpthread is the norm (e.g. Linux) and it'll be found, or we're
# on a platform that include pthreads by default (e.g. BSD, macOS) where this
# won't find anything, or we're on a road-much-less-traveled OS where the user
# can figure out what's wrong without a hard error here.
find_package(Threads)
find_package(Boost 1.53.0 COMPONENTS system program_options date_time filesystem REQUIRED)

#---------------------------------------------------------------------------
# Enumerate input files.
#---------------------------------------------------------------------------

set(CADABRA_SERVER_SRC
   cadabra-server.cc
   Server.cc
   Snoop.cc
   ${CADABRA_CORE_DIR}/InstallPrefix.cc
   ${CADABRA_CORE_DIR}/DataCell.cc
   ${CADABRA_CORE_DIR}/Exceptions.cc
   ${CADABRA_CORE_DIR}/CdbPython.cc 
   ${CADABRA_CORE_DIR}/Stopwatch.cc 
   ${CADABRA_CORE_DIR}/pythoncdb/py_helpers.cc
   ${CADABRA_LIBS_DIR}/whereami/whereami.c
   ${CADABRA_LIBS_DIR}/base64/base64.cc
   )

set(CADABRA_CLIENT_SRC
   ComputeThread.cc 
   DocumentThread.cc 
   Actions.cc 
   Snoop.cc 
   ${CADABRA_CORE_DIR}/DataCell.cc 
   ${CADABRA_CORE_DIR}/Exceptions.cc 
   ${CADABRA_CORE_DIR}/InstallPrefix.cc
   ${CADABRA_LIBS_DIR}/whereami/whereami.c
   )

set(JUPYTER_KERNEL_SRC
    cadabra-jupyter-kernel.cc
    cadabra-jupyter-kernel.hh
    cadabra-jupyter-main.cc
    Server.cc
    ${CADABRA_CORE_DIR}/InstallPrefix.cc
    ${CADABRA_CORE_DIR}/DataCell.cc
    ${CADABRA_CORE_DIR}/Exceptions.cc 
    ${CADABRA_CORE_DIR}/CdbPython.cc 
    ${CADABRA_CORE_DIR}/Stopwatch.cc 
    ${CADABRA_LIBS_DIR}/whereami/whereami.c
    ${CADABRA_LIBS_DIR}/base64/base64.cc
    )
  

# set(CONNECTION_FILE ${CMAKE_CURRENT_SOURCE_DIR}/connection.json)
# 
# add_custom_command(TARGET echo_kernel POST_BUILD
#                    COMMAND ${CMAKE_COMMAND} -E
#                    copy "${CONNECTION_FILE}" "${CMAKE_CURRENT_BINARY_DIR}/")
# 
# add_custom_command(TARGET echo_kernel POST_BUILD
#                    COMMAND ${CMAKE_COMMAND} -E
#                  copy "${CMAKE_BINARY_DIR}/${XEUS_RUNTIME_FILE}" "${CMAKE_CURRENT_BINARY_DIR}/")

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
   "${CADABRA_LIBS_DIR}/base64"
   "${CADABRA_LIBS_DIR}/nlohmann"      
   ${Boost_INCLUDE_DIRS}
   ${Python_INCLUDE_DIRS}
   ${SQLITE3_INCLUDE_DIR}
)

add_definitions(
   -D_WEBSOCKETPP_CPP11_STL_
   -DBOOST_ASIO_HAS_STD_CHRONO
   -DBOOST_BIND_GLOBAL_PLACEHOLDERS
   -DBOOST_ASIO_USE_TS_EXECUTOR_AS_DEFAULT
   -DASIO_STANDALONE
   -D_WEBSOCKETPP_CPP11_INTERNAL_
 )

#---------------------------------------------------------------------------
# Make targets.
#---------------------------------------------------------------------------

# add_executable(tst tst.cc)
# target_link_libraries(tst
#  ${Boost_LIBRARIES}
#  Threads::Threads
#  )

# Server executable
if ((NOT MSVC) OR (MSVC AND CADABRA_DEBUG_BUILD) OR (MSVC AND MSVC_TARGET_CONSOLE))
   add_executable(cadabra-server ${CADABRA_SERVER_SRC})
else()
   message(STATUS "Using the win32 subsystem")
   add_executable(cadabra-server WIN32 ${CADABRA_SERVER_SRC}) 
endif()

# target_link_libraries(cadabra-server cadabra_server)
target_link_libraries(cadabra-server
   pybind11::embed
   ${Boost_LIBRARIES}
   ${SQLITE3_LIBRARIES}
   ${Python_LIBRARIES}
   ${GLIBMM3_LIBRARIES} 
   Threads::Threads
   )
if(WIN32)
   target_link_libraries(cadabra-server ws2_32 mswsock bcrypt)
 endif()
 if(UNIX)
    if(${CMAKE_SYSTEM_NAME} MATCHES ".*BSD|DragonFly")
       target_link_libraries(cadabra-server util)
    else()
       target_link_libraries(cadabra-server dl util)
    endif()
 endif()

# Client library
add_library(cadabra_client STATIC ${CADABRA_CLIENT_SRC})
target_link_libraries(cadabra_client
  ${Boost_LIBRARIES}
  ${SQLITE3_LIBRARIES}
  ${Python_LIBRARIES}
  ${GLIBMM3_LIBRARIES}
  Threads::Threads
  )
if(WIN32)
  target_link_libraries(cadabra_client ws2_32 bcrypt)
endif()

# cadabra2html
add_executable(cadabra2html 
  cadabra2html.cc 
  ${CADABRA_LIBS_DIR}/base64/base64.cc
  )
target_link_libraries(cadabra2html cadabra_client)

# cadabra2latex
add_executable(cadabra2latex 
  cadabra2latex.cc 
  ${CADABRA_LIBS_DIR}/base64/base64.cc
  )
target_link_libraries(cadabra2latex cadabra_client)

# Jupyter kernel
# if(ENABLE_JUPYTER)
#   print_header("Configuring Xeus Jupyter kernel")
#   find_package(xtl REQUIRED)
#   find_package(nlohmann_json REQUIRED)
#   find_library(UUID_LIBRARIES uuid REQUIRED)
#   include_directories(${CMAKE_INSTALL_PREFIX}/include ${XTL_INCLUDE_DIR} ${NLOHMANN_JSON_INCLUDE_DIR})
#   link_directories(${CMAKE_INSTALL_PREFIX}/lib)
#   add_executable(cadabra-jupyter-kernel ${JUPYTER_KERNEL_SRC})
#   target_compile_definitions(cadabra-jupyter-kernel PRIVATE CDB_DONT_ACQUIRE_GIL=1)  
#   message("-- UUID library at ${UUID_LIBRARIES}")
#   target_link_libraries(cadabra-jupyter-kernel xeus Threads::Threads uuid ${Boost_LIBRARIES} ${GLIBMM3_LIBRARIES} ${Python_LIBRARIES})
# else()
#   message("-- Not building Xeus Jupyter kernel")
# endif()

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

install(TARGETS cadabra-server             DESTINATION ${CDB_BIN_PATH})
install(TARGETS cadabra2html cadabra2latex DESTINATION ${CDB_BIN_PATH})

# We don't need to install the libcadabra_client library as it is statically
# linked with our binaries anyway.
# install(
#  TARGETS cadabra_client 
#  RUNTIME DESTINATION bin COMPONENT runtime 
#  LIBRARY ARCHIVE DESTINATION lib 
#  LIBRARY DESTINATION lib
# )
# install(TARGETS cadabra_server LIBRARY ARCHIVE DESTINATION lib LIBRARY DESTINATION lib)

if(ENABLE_JUPYTER)
  install(TARGETS cadabra-jupyter-kernel      DESTINATION ${CDB_BIN_PATH})
  install(FILES   kernel.json connection.json DESTINATION share/jupyter/kernels/cadabra)
endif()

if (MSVC AND NOT INSTALL_TARGETS_ONLY)
   # GTK helpers
   install(
      FILES
         "${_VCPKG_ROOT_DIR}/installed/x64-windows/tools/glib/gspawn-win64-helper.exe"    
         "${_VCPKG_ROOT_DIR}/installed/x64-windows/tools/glib/gspawn-win64-helper-console.exe"
      DESTINATION
         ${CDB_BIN_PATH}
         )
   # Python (for some reason this is not copied in)
   get_filename_component(CDB_PYTHON_BASE ${Python_LIBRARIES} NAME_WE)
   get_filename_component(CDB_PYTHON_PATH ${Python_LIBRARIES} DIRECTORY)
   message("--- Python directory: ${CDB_PYTHON_PATH}")
   string(TOLOWER ${CDB_PYTHON_BASE} CDB_PYTHON_BASE)
   message("--- Python library:   ${CDB_PYTHON_BASE}.dll")
   install(FILES "${CDB_PYTHON_PATH}/../${CDB_PYTHON_BASE}.dll"
     DESTINATION    ${CDB_BIN_PATH}
   )
endif()

# manual pages
if(NOT MSVC)
  install(
    FILES
    ../man/man1/cadabra-server.1
    DESTINATION
    share/man/man1
    )
endif()
