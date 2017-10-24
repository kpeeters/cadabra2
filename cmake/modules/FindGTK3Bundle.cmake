# GTK 

if(WIN32)
	message("--- Looking for gtk dependencies in ${GTK3_BUNDLE_INCLUDE_DIR_SEARCH} and ${GTK3_BUNDLE_LIBRARIES_SEARCH}")
	find_path(GLIB_INCLUDE_DIR NAMES glib.h
				HINTS ${GTK3_BUNDLE_INCLUDE_DIR_SEARCH}
				PATH_SUFFIXES glib-2.0
				DOC "GLIB includes directory (contains glib.h)"
				)
	find_library(GLIB_LIBRARIES NAMES glib-2.0
				HINTS ${GTK3_BUNDLE_LIBRARIES_SEARCH}
				DOC "GLIB libraries directory (contains gtkmm-3.0.lib)")

	find_path(ATK_INCLUDE_DIR NAMES atk/atk.h
				HINTS ${GTK3_BUNDLE_INCLUDE_DIR_SEARCH}
				PATH_SUFFIXES atk-1.0
				DOC "ATK includes directory (contains atk.h)"
				)
	find_library(ATK_LIBRARIES NAMES atk-1.0.lib
				HINTS ${GTK3_BUNDLE_LIBRARIES_SEARCH}
				DOC "ATK libraries directory (contains gtkmm-3.0.lib)")

	find_path(PANGO_INCLUDE_DIR NAMES pango/pango.h
				HINTS ${GTK3_BUNDLE_INCLUDE_DIR_SEARCH}
				PATH_SUFFIXES pango-1.0
				DOC "PANGO includes directory (contains pango.h)"
				)
	find_library(PANGO_LIBRARIES NAMES pango-1.0.lib
				HINTS ${GTK3_BUNDLE_LIBRARIES_SEARCH}
				DOC "PANGO libraries directory (contains gtkmm-3.0.lib)")

	find_path(GIO_INCLUDE_DIR NAMES gio/gwin32inputstream.h
				HINTS ${GTK3_BUNDLE_INCLUDE_DIR_SEARCH}
				PATH_SUFFIXES gio-win32-2.0
				DOC "GIO includes directory (contains gwin32inputstream.h)"
				)
	find_library(GIO_LIBRARIES NAMES gio-2.0.lib
				HINTS ${GTK3_BUNDLE_LIBRARIES_SEARCH}
				DOC "GIO libraries directory (contains gtkmm-3.0.lib)")

	find_path(CAIRO_INCLUDE_DIR NAMES cairo.h
				HINTS ${GTK3_BUNDLE_INCLUDE_DIR_SEARCH}
				DOC "CAIRO includes directory (contains cairo.h)"
				)
	find_library(CAIRO_LIBRARIES NAMES cairo.lib
				HINTS ${GTK3_BUNDLE_LIBRARIES_SEARCH}
				DOC "CAIRO libraries directory (contains gtkmm-3.0.lib)")

	find_path(GDKPIXBUF_INCLUDE_DIR NAMES gdk-pixbuf/gdk-pixbuf.h
				HINTS ${GTK3_BUNDLE_INCLUDE_DIR_SEARCH}
				PATH_SUFFIXES gdk-pixbuf-2.0
				DOC "GDKPIXBUF includes directory (contains gdk-pixbuf.h)"
				)
	find_library(GDKPIXBUF_LIBRARIES NAMES gdk_pixbuf-2.0.lib
				HINTS ${GTK3_BUNDLE_LIBRARIES_SEARCH}
				DOC "GDKPIXBUF libraries directory (contains gtkmm-3.0.lib)")

	find_path(GDK_INCLUDE_DIR NAMES gdk/gdk.h
				HINTS ${GTK3_BUNDLE_INCLUDE_DIR_SEARCH}
				PATH_SUFFIXES gtk-3.0
				DOC "GDK includes directory (contains gdk.h)"
				)
	find_library(GDK_LIBRARIES NAMES gdk-3.0.lib
				HINTS ${GTK3_BUNDLE_LIBRARIES_SEARCH}
				DOC "GDK libraries directory (contains gdk-3.0.lib)")

	find_path(GTK_INCLUDE_DIR NAMES gtk/gtk.h
				HINTS ${GTK3_BUNDLE_INCLUDE_DIR_SEARCH}
				PATH_SUFFIXES gtk-3.0
				DOC "GTK includes directory (contains gtk.h)"
				)
	find_library(GTK_LIBRARIES NAMES gtk-3.0.lib
				HINTS ${GTK3_BUNDLE_LIBRARIES_SEARCH}
				DOC "GTK libraries directory (contains gtk-3.0.lib)")
				
	set(GTK3_INCLUDE_DIR "${GDKPIXBUF_INCLUDE_DIR};${CAIRO_INCLUDE_DIR};${GIO_INCLUDE_DIR};${PANGO_INCLUDE_DIR};${ATK_INCLUDE_DIR};${GLIB_INCLUDE_DIR};${GDK_INCLUDE_DIR};${GTK_INCLUDE_DIR}")

	set(GTK3_LIBRARIES "${GDKPIXBUF_LIBRARIES};${CAIRO_LIBRARIES};${GIO_LIBRARIES};${PANGO_LIBRARIES};${ATK_LIBRARIES};${GLIB_LIBRARIES};${GDK_LIBRARIES};${GTK_LIBRARIES}")
	
	
# else()
# 	find_package(PkgConfig REQUIRED)
# 	pkg_check_modules(GTK3 gtk-3.0)
endif()

find_package_handle_standard_args(GTK3 DEFAULT_MSG GTK3_INCLUDE_DIR GTK3_LIBRARIES)

if(GTK3_FOUND)
	message("--- Found GTK3 so we have ${GTK3_INCLUDE_DIR} and ${GTK3_LIBRARIES}")
else()
	message("--- Missing GTK3!!  we have ${GTK3_INCLUDE_DIR} and ${GTK3_LIBRARIES}")
endif()
