# cmake_minimum_required(VERSION ${CADABRA_CMAKE_VERSION})
project(Cadabra)


#---------------------------------------------------------------------------
# Preamble.
#---------------------------------------------------------------------------

print_header("Configuring native notebook frontend")

set(INSTALL_LATEX_DIR    "share/cadabra2/latex")
set(INSTALL_MAN_DIR      "share/cadabra2/manual")
set(INSTALL_MAN_ALGO_DIR "share/cadabra2/manual/algorithms")
set(INSTALL_MAN_PROP_DIR "share/cadabra2/manual/properties")

FILE(GLOB man_algorithms "${PROJECT_SOURCE_DIR}/../core/algorithms/*.cnb")
FILE(GLOB man_properties "${PROJECT_SOURCE_DIR}/../core/properties/*.cnb")


#---------------------------------------------------------------------------
# Locate libraries.
#---------------------------------------------------------------------------

# Toolkit detection lives in gtkmm/CMakeLists.txt, which selects between
# FindGTKMM3 and FindGTKMM4 depending on USE_GTK4. Both use
# pkg_check_modules(... REQUIRED ...), so a missing gtkmm already aborts
# the configure step with a clear message; there is nothing to check here.
add_subdirectory(gtkmm)

# if(APPLE)
#    subdirs(osx)
#    message("-- Building native OSX interface")
# else(APPLE)
#    message("-- Not building native OSX interface")
# endif(APPLE)


#---------------------------------------------------------------------------
# Installation.
#---------------------------------------------------------------------------

if (NOT INSTALL_TARGETS_ONLY)
	message(STATUS "Installing LaTeX style files in ${INSTALL_LATEX_DIR}")

	install_directory_permissions("${INSTALL_LATEX_DIR}")
	install_directory_permissions("${INSTALL_MAN_DIR}")
	install_directory_permissions("${INSTALL_MAN_ALGO_DIR}")
	install_directory_permissions("${INSTALL_MAN_PROP_DIR}")

#	install(FILES ${PROJECT_SOURCE_DIR}/latex/tableaux.sty  DESTINATION ${INSTALL_LATEX_DIR})
	install(FILES ${PROJECT_SOURCE_DIR}/common/preamble.tex DESTINATION ${INSTALL_LATEX_DIR})
	install(FILES ${man_algorithms} DESTINATION ${INSTALL_MAN_ALGO_DIR})
	install(FILES ${man_properties} DESTINATION ${INSTALL_MAN_PROP_DIR})
endif()
