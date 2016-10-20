cmake_minimum_required(VERSION 2.8)

project(Cadabra)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTKMM3 gtkmm-3.0)
subdirs(common)
if(GTKMM3_FOUND)
   subdirs(gtkmm)
   message("-- Building GTK3 user interface")
else(GTKMM3_FOUND)
   message("-- Not building GTK3 user interface")
endif(GTKMM3_FOUND)

# if(APPLE)
#    subdirs(osx)
#    message("-- Building native OSX interface")
# else(APPLE)
#    message("-- Not building native OSX interface")
# endif(APPLE)

execute_process(COMMAND kpsewhich -var-value=TEXMFLOCAL OUTPUT_VARIABLE INSTALL_LATEX_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
# Get the last path in the LaTeX install dir, if there is more than one separated by a ':'.
#string(REPLACE ":" ";" INSTALL_LATEX_DIR "${INSTALL_LATEX_DIR}")
#list(GET INSTALL_LATEX_DIR -1 INSTALL_LATEX_DIR)
set(INSTALL_LATEX_DIR "${INSTALL_LATEX_DIR}/tex/latex/cadabra2")
message("-- Installing LaTeX style files in ${INSTALL_LATEX_DIR}")
install(DIRECTORY DESTINATION ${INSTALL_LATEX_DIR} DIRECTORY_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE 
                                                             GROUP_READ GROUP_EXECUTE 
                                                             WORLD_READ WORLD_EXECUTE)
install(FILES    
  ${PROJECT_SOURCE_DIR}/latex/tableaux.sty 
#  ${PROJECT_SOURCE_DIR}/latex/msabm.sym
#  ${PROJECT_SOURCE_DIR}/latex/mathstyle.sty 
#  ${PROJECT_SOURCE_DIR}/latex/mathptmx.sym
#  ${PROJECT_SOURCE_DIR}/latex/mathpazo.sym 
#  ${PROJECT_SOURCE_DIR}/latex/flexisym.sty 
#  ${PROJECT_SOURCE_DIR}/latex/cmbase.sym 
#  ${PROJECT_SOURCE_DIR}/latex/breqn.sty 
  DESTINATION ${INSTALL_LATEX_DIR})

install(SCRIPT   ${PROJECT_SOURCE_DIR}/latex/install.cmake)

install(FILES  ${PROJECT_SOURCE_DIR}/common/preamble.tex DESTINATION "${CMAKE_INSTALL_PREFIX}/share/cadabra2/texengine/")

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
