if(WIN33)
#  windows_find_library(GTKMM3 "gtkmm.dll" "glib;sigc++;pango")
  windows_find_library(GTKMM3_LIBRARIES
	gtk gdk gdk_pixbuf pangocairo pango atk gio gobject
	gmodule glib cairo-gobject cairo intl atkmm cairomm
	gdkmm giomm glibmm gtkmm pangomm
  )
  if (GTKMM3_LIBRARIES)
    set(GTKMM3_FOUND TRUE)
  endif()
else()
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(GTKMM3 REQUIRED gtkmm-3.0)
  pkg_check_modules(PANGOMM REQUIRED pangomm-1.4)
  include_directories(${GTKMM3_INCLUDE_DIRS} ${PANGOMM_INCLUDE_DIRS})
  link_directories(${GTKMM3_LIBRARY_DIRS} ${PANGOMM_LIBRARY_DIRS})
  add_definitions(${GTKMM3_CFLAGS_OTHER} ${PANGOMM_CFLAGS_OTHER})
endif()

if (GTKMM3_FOUND)
  message(STATUS "Found gtkmm")
endif()
