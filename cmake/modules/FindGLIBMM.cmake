#
# Find Glibmm libraries
#   On Windows, we first check if vcpkg is installed. If it is,
#    we query whether it has glibmm listed under its installed
#    packages. If so we can query it for include and library
#    locations. If vcpkg is not installed or it does not listed
#    glibmm we check the GLIBMM_ROOT variable and if it is set
#    we attempt to find glibmm from there.
#   On linux and mac we query pkgconfig for the location of
#    glibmm and its dependencies

if(WIN32)
  windows_find_library(GLIBMM "glibmm.dll" "glib;sigc++")
  set(GLIBMM3_LIBRARIES ${GLIBMM_LIBRARIES})
else()
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(GLIBMM3 REQUIRED glibmm-2.4)
  include_directories(${GLIBMM3_INCLUDE_DIRS})
  link_directories(${GLIBMM3_LIBRARY_DIRS})
  add_definitions(${GLIBMM3_CFLAGS_OTHER})
endif()
