if(WIN32)
#  windows_find_library(GTKMM3 "gtkmm.dll" "glib;sigc++;pango")
  windows_find_library(GTKMM4_LIBRARIES
	gtk gdk gdk_pixbuf pangocairo pango atk gio gobject
	gmodule glib cairo-gobject cairo intl atkmm cairomm
	gdkmm giomm glibmm gtkmm pangomm
  )
  if (GTKMM4_LIBRARIES)
    set(GTKMM4_FOUND TRUE)
  endif()
else()
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(GTKMM4  REQUIRED IMPORTED_TARGET gtkmm-4.0)
  pkg_check_modules(CairoMM REQUIRED IMPORTED_TARGET cairomm-1.16)
  include_directories(${GTKMM4_INCLUDE_DIRS})
  link_directories(${GTKMM4_LIBRARY_DIRS})
  add_definitions(${GTKMM4_CFLAGS_OTHER})
endif()

if (GTKMM4_FOUND)
  message(STATUS "Found gtkmm4")
endif()
