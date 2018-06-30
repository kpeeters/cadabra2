cmake_minimum_required(VERSION 3.1)
project(Cadabra)


#---------------------------------------------------------------------------
# Preamble.
#---------------------------------------------------------------------------

print_header("Configuring frontend")


#---------------------------------------------------------------------------
# Locate libraries.
#---------------------------------------------------------------------------

find_package(GTKMM3)
if(GTKMM3_FOUND)
   add_subdirectory(gtkmm)
else()
	message(FATAL_ERROR "ENABLE_FRONTEND is set to ON, but no supported frontend libraries were found")
endif()

# if(APPLE)
#    subdirs(osx)
#    message("-- Building native OSX interface")
# else(APPLE)
#    message("-- Not building native OSX interface")
# endif(APPLE)


#---------------------------------------------------------------------------
# Installation.
#---------------------------------------------------------------------------

set(INSTALL_LATEX_DIR "${CMAKE_INSTALL_PREFIX}/share/cadabra2/latex")
message(STATUS "Installing LaTeX style files in ${INSTALL_LATEX_DIR}")
install(DIRECTORY DESTINATION ${INSTALL_LATEX_DIR} DIRECTORY_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE 
                                                             GROUP_READ GROUP_EXECUTE 
                                                             WORLD_READ WORLD_EXECUTE)

install(FILES ${PROJECT_SOURCE_DIR}/latex/tableaux.sty  DESTINATION ${INSTALL_LATEX_DIR})
install(FILES ${PROJECT_SOURCE_DIR}/common/preamble.tex DESTINATION ${INSTALL_LATEX_DIR})

set(INSTALL_MAN_DIR      "${CMAKE_INSTALL_PREFIX}/share/cadabra2/manual")
set(INSTALL_MAN_ALGO_DIR "${CMAKE_INSTALL_PREFIX}/share/cadabra2/manual/algorithms")
set(INSTALL_MAN_PROP_DIR "${CMAKE_INSTALL_PREFIX}/share/cadabra2/manual/properties")
install(DIRECTORY DESTINATION ${INSTALL_MAN_DIR} DIRECTORY_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE 
                                                             GROUP_READ GROUP_EXECUTE 
                                                             WORLD_READ WORLD_EXECUTE)
install(DIRECTORY DESTINATION ${INSTALL_MAN_ALGO_DIR} DIRECTORY_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE 
                                                             GROUP_READ GROUP_EXECUTE 
                                                             WORLD_READ WORLD_EXECUTE)
install(DIRECTORY DESTINATION ${INSTALL_MAN_PROP_DIR} DIRECTORY_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE 
                                                             GROUP_READ GROUP_EXECUTE 
                                                             WORLD_READ WORLD_EXECUTE)

FILE(GLOB man_algorithms "${PROJECT_SOURCE_DIR}/../core/algorithms/*.cnb")
install(FILES ${man_algorithms} DESTINATION ${INSTALL_MAN_ALGO_DIR})
FILE(GLOB man_properties "${PROJECT_SOURCE_DIR}/../core/properties/*.cnb")
install(FILES ${man_properties} DESTINATION ${INSTALL_MAN_PROP_DIR})
