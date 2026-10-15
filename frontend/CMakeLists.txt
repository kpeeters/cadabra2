# cmake_minimum_required(VERSION ${CADABRA_CMAKE_VERSION})
project(Cadabra)


#---------------------------------------------------------------------------
# Preamble.
#---------------------------------------------------------------------------

print_header("Configuring native notebook frontend")

# The manual pages are installed by core/CMakeLists.txt.
set(INSTALL_LATEX_DIR    "share/cadabra2/latex")


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

#	install(FILES ${PROJECT_SOURCE_DIR}/latex/tableaux.sty  DESTINATION ${INSTALL_LATEX_DIR})
	install(FILES ${PROJECT_SOURCE_DIR}/common/preamble.tex DESTINATION ${INSTALL_LATEX_DIR})
endif()
