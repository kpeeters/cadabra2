if(WIN32)
  windows_find_library(GTKMM_LIBRARIES
	gtk gdk gdk_pixbuf pangocairo pango atk gio gobject
	gmodule glib cairo-gobject cairo intl atkmm cairomm
	gdkmm giomm glibmm gtkmm pangomm
  )
  if (GTKMM_LIBRARIES)
    set(GTKMM3_FOUND TRUE)
  endif()
else()
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(GTKMM   REQUIRED IMPORTED_TARGET gtkmm-3.0)
  pkg_check_modules(CairoMM REQUIRED IMPORTED_TARGET cairomm-1.0)
  include_directories(${GTKMM_INCLUDE_DIRS})
  link_directories(${GTKMM_LIBRARY_DIRS})
  add_definitions(${GTKMM_CFLAGS_OTHER})
  if(GTKMM_FOUND)
    set(GTKMM3_FOUND TRUE)
  endif()
endif()

if(GTKMM3_FOUND)
  message(STATUS "Found gtkmm3")
endif()
