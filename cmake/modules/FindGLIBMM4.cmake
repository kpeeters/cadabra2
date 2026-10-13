# Note: use MSVC, not WIN32, to match FindGTKMM3/FindGTKMM4; MSYS2/mingw sets
# WIN32 but does have working pkg-config, so it must take the branch below.
if(MSVC)
  windows_find_library(GLIBMM_LIBRARIES REQUIRED glibmm sigc glib gobject)
  if (GLIBMM_LIBRARIES)
    set(GLIBMM_FOUND TRUE)
  endif()
else()
  find_package(PkgConfig REQUIRED)
  # No PKG_CONFIG_PATH fixup is needed on macOS: the glibmm-2.68 /
  # cairomm-1.16 / pangomm-2.48 API versions that GTK4 wants are supplied by
  # Homebrew's *unversioned* glibmm, cairomm and pangomm formulae, and none of
  # those are keg-only, so pkg-config finds them on the default path.
  # (FindGLIBMM3 does have such a block, because gtkmm3 pulls in the keg-only
  # glibmm@2.66 / cairomm@1.14 / pangomm@2.46.)
  pkg_check_modules(GLIBMM REQUIRED glibmm-2.68)
  include_directories(${GLIBMM_INCLUDE_DIRS})
  link_directories(${GLIBMM_LIBRARY_DIRS})
  add_definitions(${GLIBMM_CFLAGS_OTHER})
endif()

if (GLIBMM_FOUND)
  # find_package(GLIBMM4 REQUIRED) checks GLIBMM4_FOUND, not GLIBMM_FOUND.
  set(GLIBMM4_FOUND TRUE)
  message(STATUS "Found glibmm; include path ${GLIBMM_INCLUDE_DIRS}")
  message(STATUS "Found glibmm; lib ${GLIBMM_LIBRARIES}")
endif()
