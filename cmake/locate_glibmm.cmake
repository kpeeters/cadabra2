#
# Find Glibmm libraries
#

if(WIN32)
  find_path(GLIB_INCLUDE_DIR glib.h HINTS ${GLIBMM_ROOT}\\include\\glib)
  find_path(GLIB_CONFIG_DIR glibconfig.h HINTS ${GLIBMM_ROOT}\\lib\\glib\\include)
  find_library(GLIB_LIBRARY glib HINTS ${GLIBMM_ROOT}\\lib)
  find_path(GLIBMM_INCLUDE_DIR glibmm.h HINTS ${GLIBMM_ROOT}\\include\\glibmm)
  find_path(GLIBMM_CONFIG_DIR glibmmconfig.h HINTS ${GLIBMM_ROOT}\\lib\\glibmm\\include)
  find_library(GLIBMM_LIBRARY glibmm HINTS ${GLIBMM_ROOT}\\lib)
  find_path(SIGCPP_INCLUDE_DIR sigc++.h HINTS ${GLIBMM_ROOT}\\include\\sigc++)
  find_path(SIGCPP_CONFIG_DIR sigc++config.h HINTS ${GLIBMM_ROOT}\\lib\\sigc++\\include)
  find_library(SIGCPP_LIBRARY sigc++ HINTS ${GLIBMM_ROOT}\\lib)
  
  set(GLIBMM3_INCLUDE_DIRS 
    ${GLIB_INCLUDE_DIR}
	${GLIB_CONFIG_DIR}
	${GLIBMM_INCLUDE_DIR}
	${GLIBMM_CONFIG_DIR}
	${SIGCPP_INCLUDE_DIR}
	${SIGCPP_CONFIG_DIR}
  )
  
  set(GLIBMM3_LIBRARIES
    ${GLIB_LIBRARY}
	${GLIBMM_LIBRARY}
	${SIGCPP_LIBRARY}
  )
  
  include_directories(${GLIBMM3_INCLUDE_DIRS})
  
else()
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(GLIBMM3 REQUIRED glibmm-2.4)
  include_directories(${GLIBMM3_INCLUDE_DIRS})
  link_directories(${GLIBMM3_LIBRARY_DIRS})
  add_definitions(${GLIBMM3_CFLAGS_OTHER})
endif()
message("-- Found Glibmm3 ${GLIBMM3_LIBRARIES}")