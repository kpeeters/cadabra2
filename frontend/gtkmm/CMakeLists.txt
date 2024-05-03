cmake_minimum_required(VERSION ${CADABRA_CMAKE_VERSION})
project(CadabraGtkmm)


#---------------------------------------------------------------------------
# Preamble.
#---------------------------------------------------------------------------

message(STATUS "Using gtkmm frontend")
if(USE_MICROTEX)
  message(STATUS "Including MicroTeX support for typesetting")
else()
  message(STATUS "Only using LaTeX for typesetting (disabling MicroTeX)")
endif()

set(INSTALL_SHARE_DIR share/cadabra2 CACHE PATH "Installation directory for shared files.")
set(INSTALL_IMAGES_DIR share/cadabra2/images CACHE PATH "Installation directory for image files.")
set(INSTALL_DESKTOP_DIR share/applications CACHE PATH "Installation directory for .desktop files.")
set(INSTALL_ICONS_DIR share/icons CACHE PATH "Installation directory for icons.")
set(INSTALL_MICROTEX_DIR "${CMAKE_INSTALL_PREFIX}/share/cadabra2/microtex")

#---------------------------------------------------------------------------
# Locate libraries.
#---------------------------------------------------------------------------

find_package(Threads)
find_package(SQLITE3)
find_package(GLIBMM)
find_package(GTKMM3)
find_package(Boost 1.45.0 COMPONENTS system)
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)


#---------------------------------------------------------------------------
# Enumerate input files and directories.
#---------------------------------------------------------------------------

set(cadabra_gtk_src 
	main.cc 
	ChooseColoursDialog.cc
	DiffViewer.cc
	NotebookWindow.cc 
	NotebookCanvas.cc 
	SelectFileDialog.cc
	TeXView.cc 
	CodeInput.cc 
	ImageView.cc
	Cadabra.cc
	Console.cc
	Keywords.cc 
	../common/TeXEngine.cc
	../common/lodepng.cc
	# ../common/exec-stream.cc
	${CADABRA_LIBS_DIR}/tiny-process-library/process.cpp
	${CADABRA_LIBS_DIR}/base64/base64.cc
)
if (MSVC)
	list(APPEND cadabra_gtk_src ${CADABRA_LIBS_DIR}/tiny-process-library/process_win.cpp)
	# Configure and add resource file to list of sources
	configure_file(win_res.rc.in ${CMAKE_CURRENT_BINARY_DIR}/win_res.rc)
	list(APPEND cadabra_gtk_src ${CMAKE_CURRENT_BINARY_DIR}/win_res.rc)
else()
	list(APPEND cadabra_gtk_src ${CADABRA_LIBS_DIR}/tiny-process-library/process_unix.cpp)
endif()



#---------------------------------------------------------------------------
# Include directories and preprocessor definitions.
#---------------------------------------------------------------------------

include_directories(
	"."
	"${CADABRA_CORE_DIR}"
	"${CADABRA_CLIENT_SERVER_DIR}"
	"${CADABRA_LIBS_DIR}/internal/include"
	"${CADABRA_LIBS_DIR}/websocketpp"
	"${CADABRA_LIBS_DIR}/tiny-process-library"
	"${CADABRA_LIBS_DIR}/nlohmann"   
	${Boost_INCLUDE_DIRS}
	${SQLITE3_INCLUDE_DIR}  
	)

if(ENABLE_SYSTEM_JSONCPP)
  include_directories(
	 ${JSONCPP_INCLUDE_DIRS}
	 )
else()
  include_directories(
	  "${CADABRA_LIBS_DIR}/jsoncpp"
	  )
endif()
 
add_definitions(
	"-D_WEBSOCKETPP_CPP11_STL_"
	"-DBOOST_LOG_DYN_LINK"
	"-DGTKMM_DISABLE_DEPRECATED"
	"-DBUILD_GTK"
	"-Wno-reorder"
)

#---------------------------------------------------------------------------
# Targets.
#---------------------------------------------------------------------------

if ((NOT MSVC) OR (MSVC AND CADABRA_DEBUG_BUILD) OR (MSVC AND MSVC_TARGET_CONSOLE))
	add_executable(cadabra2-gtk ${cadabra_gtk_src})
else()
	add_executable(cadabra2-gtk WIN32 ${cadabra_gtk_src})
endif()

if(USE_MICROTEX)
  message(STATUS "INCLUDE ${PROJECT_SOURCE_DIR}/../../libs/microtex/src")
  include_directories(
    #  ${GSVMM_INCLUDES}
    ${PROJECT_SOURCE_DIR}/../../libs/microtex/src
    ${PROJECT_SOURCE_DIR}/../../libs/microtex/src/samples 
  )
  find_package(Fontconfig REQUIRED)
#  pkg_check_modules(tinyxml2 REQUIRED IMPORTED_TARGET tinyxml2)
endif()
pkg_check_modules(GTKMM REQUIRED IMPORTED_TARGET gtkmm-3.0)
# pkg_check_modules(GSVMM REQUIRED IMPORTED_TARGET gtksourceviewmm-3.0)
pkg_check_modules(CairoMM REQUIRED IMPORTED_TARGET cairomm-1.0)
# add_executable(microtest microtest.cc)

target_link_libraries(
  cadabra2-gtk 
  cadabra_client 
  ${Boost_LIBRARIES} 
  ${GLIBMM_LIBRARIES} 
  ${GTKMM3_LIBRARIES} 
  ${SQLITE3_LIBRARIES} 
  Threads::Threads
)

if(USE_MICROTEX)
  target_link_libraries(cadabra2-gtk 
    LaTeX                   # from MicroTeX
    PkgConfig::CairoMM      # for MicroTeX
    Fontconfig::Fontconfig  # for MicroTeX
    tinyxml2                # for MicroTeX
  )
endif()

# target_link_libraries(microtest 
# 	${PROJECT_SOURCE_DIR}/../../libs/microtex/build/libLaTeX.a
# 	${GLIBMM_LIBRARIES} 
# 	${GTKMM3_LIBRARIES} 
# 	Threads::Threads
# 	PkgConfig::GTKMM
#         PkgConfig::CairoMM
# 	PkgConfig::GSVMM
#         Fontconfig::Fontconfig
# 	tinyxml2
# 	Threads::Threads
# )


#---------------------------------------------------------------------------
# Installation.
#---------------------------------------------------------------------------

install(TARGETS cadabra2-gtk DESTINATION bin)

# Create installation directories with correct permissions.
if (NOT INSTALL_TARGETS_ONLY)
	install_directory_permissions(${INSTALL_SHARE_DIR})
	install_directory_permissions(${INSTALL_IMAGES_DIR})
	install_directory_permissions(${INSTALL_DESKTOP_DIR})
	install_directory_permissions(${INSTALL_ICONS_DIR})
	install_directory_permissions("${INSTALL_ICONS_DIR}/hicolor")
	install_directory_permissions("${INSTALL_ICONS_DIR}/hicolor/64x64")
	install_directory_permissions("${INSTALL_ICONS_DIR}/hicolor/256x256")
	install_directory_permissions("${INSTALL_ICONS_DIR}/hicolor/scalable")
	install_directory_permissions("${INSTALL_ICONS_DIR}/hicolor/64x64/apps")
	install_directory_permissions("${INSTALL_ICONS_DIR}/hicolor/256x256/apps")
	install_directory_permissions("${INSTALL_ICONS_DIR}/hicolor/scalable/apps")

	install(FILES ${CADABRA_IMAGES_DIR}/256x256/cadabra2-gtk.png DESTINATION ${INSTALL_IMAGES_DIR})

	#xdg-desktop-menu messes up the file permissions, so we just install things by hand...
	#install(CODE "execute_process(COMMAND ${XDG-DESKTOP-MENU_EXECUTABLE} install --novendor ${PROJECT_SOURCE_DIR}/../../config/cadabra2.desktop)")

	# Icons are a nightmare...
	# 
	#   - You cannot just plainly install in /usr/share/icons/hicolor because that is not where Homebrew/OS X
	#     stores the icons (it uses /usr/local/share/icons/hicolor).
	#   - /usr/local/share/icons/hicolor may lack an index.theme file, which means that things below there
	#     actually never get picked up (or at least not by cinnamon).
	#   - So we need to copy index.theme into /usr/local if it does not exist there yet.
	#   - In order to avoid all that, just use /usr/share/icons on Linux; package managers will install it
	#     there anyway.
	#   - gtk-update-icon-cache is broken and only works half of the time.
	#
	# BUT: package managers really do not like this approach, so we're back to doing the proper thing and
	# hope window managers will eventually fix this.
	
	set(ICON_PREFIX "${CMAKE_INSTALL_PREFIX}")

	set(ICON_PREFIX_WITH_DESTDIR $ENV{DESTDIR}${ICON_PREFIX})
	install(FILES    ${CADABRA_IMAGES_DIR}/64x64/cadabra2-gtk.png     DESTINATION ${ICON_PREFIX}/share/icons/hicolor/64x64/apps)
	install(FILES    ${CADABRA_IMAGES_DIR}/128x128/cadabra2-gtk.png   DESTINATION ${ICON_PREFIX}/share/icons/hicolor/128x128/apps)
	install(FILES    ${CADABRA_IMAGES_DIR}/256x256/cadabra2-gtk.png   DESTINATION ${ICON_PREFIX}/share/icons/hicolor/256x256/apps)
	install(FILES    ${CADABRA_IMAGES_DIR}/cadabra2-gtk.svg           DESTINATION ${ICON_PREFIX}/share/icons/hicolor/scalable/apps)
	install(FILES    ${CADABRA_ROOT_DIR}/config/science.cadabra.cadabra2-gtk.desktop  DESTINATION ${ICON_PREFIX}/share/applications)
	install(CODE     "execute_process(COMMAND touch ${ICON_PREFIX_WITH_DESTDIR}/share/icons/hicolor/icon-theme.cache)")
	install(CODE     "execute_process(COMMAND gtk-update-icon-cache-3.0 {ICON_PREFIX_WITH_DESTDIR}/share/icons/hicolor)")
	install(CODE     "execute_process(COMMAND chmod go+r ${ICON_PREFIX_WITH_DESTDIR}/share/icons/hicolor/icon-theme.cache)")
	install(DIRECTORY cdb-icons  DESTINATION ${CMAKE_INSTALL_PREFIX}/share/cadabra2/)  

	# Install shared libraries on Windows. 
	# https://stackoverflow.com/questions/32662215/how-do-you-install-gtk-3-0-on-windows
	if(WIN32)
		# All things to make GTK apps work and look decent on Windows 10.
		# We always need the Adwaita theme for fallback icons, but VCPKG
		# does not yet distribute it, so we include it here (also see
		# https://github.com/Microsoft/vcpkg/pull/693). The Windows 10 theme
		# comes from https://github.com/B00merang-Project/Windows-10.
		# Settings as suggested by Luca Bacci on gtk-devel-list.
		install(FILES     config/gschemas.compiled DESTINATION ${CMAKE_INSTALL_PREFIX}/share/glib-2.0/schemas)
		install(FILES     config/settings.ini      DESTINATION ${CMAKE_INSTALL_PREFIX}/etc/gtk-3.0/)  
		install(DIRECTORY theme/Windows10          DESTINATION ${CMAKE_INSTALL_PREFIX}/share/themes)
		install(DIRECTORY icons/Adwaita            DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons)
		install(DIRECTORY icons/hicolor            DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons)  
		install_dlls_from("frontend/gtkmm")
	endif()
endif()

# manual pages
if(NOT MSVC)
  install(
	 FILES
	 ../../man/man1/cadabra2-gtk.1
	 DESTINATION
	 share/man/man1
	 )
endif()

# microtex resources
if(USE_MICROTEX)
  install(DIRECTORY ${PROJECT_SOURCE_DIR}/../../libs/microtex/res/ DESTINATION ${INSTALL_MICROTEX_DIR})
  install(FILES     ${PROJECT_SOURCE_DIR}/science.cadabra.cadabra2-gtk.appdata.xml DESTINATION ${CMAKE_INSTALL_PREFIX}/share/metainfo/)
  install(CODE      "execute_process(COMMAND touch \$ENV{DESTDIR}${INSTALL_MICROTEX_DIR}/.clatexmath-res_root)")
endif()

