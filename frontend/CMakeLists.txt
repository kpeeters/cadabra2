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

set(INSTALL_LATEX_DIR     share/texmf/tex/latex/cadabra2 CACHE PATH "Installation directory for LaTeX packages.")
install(DIRECTORY DESTINATION ${INSTALL_LATEX_DIR} DIRECTORY_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE 
                                                             GROUP_READ GROUP_EXECUTE 
                                                             WORLD_READ WORLD_EXECUTE)
install(FILES    ${PROJECT_SOURCE_DIR}/latex/tableaux.sty DESTINATION ${INSTALL_LATEX_DIR})
install(SCRIPT   ${PROJECT_SOURCE_DIR}/latex/install.cmake)
