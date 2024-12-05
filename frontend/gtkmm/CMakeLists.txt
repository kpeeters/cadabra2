cmake_minimum_required(VERSION ${CADABRA_CMAKE_VERSION})
project(CadabraGtkmm)
include(FetchContent)

if(POLICY CMP0167)
  cmake_policy(SET CMP0167 NEW)
endif()
if(POLICY CMP0177)
  cmake_policy(SET CMP0177 NEW)
endif()

#---------------------------------------------------------------------------
# Preamble.
#---------------------------------------------------------------------------

message(STATUS "Using gtkmm frontend")
if(USE_MICROTEX)
  message(STATUS "Including MicroTeX support for typesetting")
  set(CAIRO TRUE)
  set(HAVE_LOG FALSE)
  set(GRAPHICS_DEBUG FALSE)
  set(_GLYPH_RENDER_TYPE 2)  # force rendering using otf font, not paths1
  set(BUILD_STATIC ON)

  set(MICROTEX_SUBMODULE ${CMAKE_SOURCE_DIR}/submodules/microtex/)
  if(EXISTS ${MICROTEX_SUBMODULE}/CMakeLists.txt)
    message(STATUS "Found MicroTeX as submodule in ${MICROTEX_SUBMODULE}")
    # Someone has already checked out the microtex submodule, use that.
    set(microtex_POPULATED TRUE)
    set(microtex_SOURCE_DIR ${MICROTEX_SUBMODULE})
    set(microtex_BINARY_DIR ${microtex_SOURCE_DIR}/build)
    add_subdirectory(${microtex_SOURCE_DIR} ${microtex_BINARY_DIR} EXCLUDE_FROM_ALL)
  else()
    # Fetch microtex as we do not have it yet.
    message(STATUS "Fetching MicroTeX using FetchContent")    
    FetchContent_Declare(
      microtex
      GIT_REPOSITORY https://github.com/kpeeters/MicroTeX.git
      # GIT_REPOSITORY file:///home/kasper/git/microtex/
      GIT_TAG        kpeeters/cadabra
    )
    if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.14")
      FetchContent_MakeAvailable(microtex)
    else()
      FetchContent_GetProperties(microtex)
      if(NOT microtex_POPULATED)
	FetchContent_Populate(microtex)
	add_subdirectory(${microtex_SOURCE_DIR} ${microtex_BINARY_DIR} EXCLUDE_FROM_ALL)
      endif()
    endif()
  endif()
  
  target_compile_options(microtex PUBLIC "-DHAVE_AUTO_FONT_FIND=1" "-Wno-reorder" "-Wno-sign-compare" "-Wno-switch" "-Wno-unused-variable" "-Wno-unused-parameter" "-Wno-ignored-qualifiers" "-Wno-mismatched-tags" "-Wno-missing-braces" "-Wno-missing-field-initializers")
else()
  message(STATUS "Only using LaTeX for typesetting (disabling MicroTeX)")
endif()

set(INSTALL_SHARE_DIR share/cadabra2 CACHE PATH "Installation directory for shared files.")
set(INSTALL_IMAGES_DIR share/cadabra2/images CACHE PATH "Installation directory for image files.")
set(INSTALL_DESKTOP_DIR share/applications CACHE PATH "Installation directory for .desktop files.")
set(INSTALL_ICONS_DIR share/icons CACHE PATH "Installation directory for icons.")
set(INSTALL_MICROTEX_DIR "share/cadabra2/microtex")

#---------------------------------------------------------------------------
# Locate libraries.
#---------------------------------------------------------------------------

find_package(Threads)
find_package(SQLITE3)
find_package(GLIBMM)
find_package(GTKMM3)
find_package(Boost 1.71.0 COMPONENTS system REQUIRED)
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
  ${CADABRA_CORE_DIR}/CdbPython.cc 
  ${CADABRA_LIBS_DIR}/tiny-process-library/process.cpp
  ${CADABRA_LIBS_DIR}/base64/base64.cc
)
if(WIN32)
  list(APPEND cadabra_gtk_src ${CADABRA_LIBS_DIR}/tiny-process-library/process_win.cpp)
  # Configure and add resource file to list of sources
  configure_file(win_res.rc.in ${CMAKE_CURRENT_BINARY_DIR}/win_res.rc)
  add_custom_command(
    OUTPUT win_res.o
    COMMAND windres ${CMAKE_CURRENT_BINARY_DIR}/win_res.rc ${CMAKE_CURRENT_BINARY_DIR}/win_res.o
    COMMENT "Convert .rc file to .o to give us an icon."
  )
  list(APPEND cadabra_gtk_src ${CMAKE_CURRENT_BINARY_DIR}/win_res.o)
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
  "${CADABRA_LIBS_DIR}/pybind11/include"
  "${CADABRA_LIBS_DIR}/internal/include"
  "${CADABRA_LIBS_DIR}/websocketpp"
  "${CADABRA_LIBS_DIR}/tiny-process-library"
  "${CADABRA_LIBS_DIR}/nlohmann"   
  ${Boost_INCLUDE_DIRS}
  ${Python_INCLUDE_DIRS}
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
  message(STATUS "MicroTeX binaries will be built at ${microtex_BINARY_DIR}")
  include_directories(
    ${microtex_BINARY_DIR}/lib
    ${microtex_SOURCE_DIR}/lib
    ${microtex_SOURCE_DIR}/platform    
  )
  # find_package(Fontconfig REQUIRED)
  pkg_check_modules(Fontconfig REQUIRED IMPORTED_TARGET fontconfig)
endif()
pkg_check_modules(CairoMM REQUIRED IMPORTED_TARGET cairomm-1.0)
pkg_check_modules(PangoMM REQUIRED IMPORTED_TARGET pangomm-1.4)

target_compile_options(
  cadabra2-gtk
  PUBLIC
  ${Boost_CFLAGS_OTHER} 
  ${GLIBMM_CFLAGS_OTHER} 
  ${GTKMM3_CFLAGS_OTHER} 
  ${SQLITE3_CFLAGS_OTHER} 
  ${Fontconfig_CFLAGS_OTHER}
)
target_link_libraries(
  cadabra2-gtk
  PUBLIC
  cadabra_client 
  ${Boost_LIBRARIES} 
  ${GLIBMM_LIBRARIES} 
  ${GTKMM3_LIBRARIES} 
  ${SQLITE3_LIBRARIES} 
  Threads::Threads
)

if(USE_MICROTEX)
  if(WIN32)
    target_link_libraries(
      cadabra2-gtk
      PUBLIC
      microtex-cairo          # from MicroTeX
      microtex                # from MicroTeX
      ${CairoMM_LIBRARIES}
      ${PangoMM_LIBRARIES}
      ${Fontconfig_LIBRARIES}
    )
  else()
    target_link_libraries(
      cadabra2-gtk
      PUBLIC
      microtex-cairo          # from MicroTeX
      microtex                # from MicroTeX
      PkgConfig::CairoMM
      PkgConfig::PangoMM
      PkgConfig::Fontconfig
    )
  endif()
endif()


#---------------------------------------------------------------------------
# Installation.
#---------------------------------------------------------------------------

install(TARGETS cadabra2-gtk DESTINATION ${CDB_BIN_PATH})

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
  
  set(ICON_PREFIX ".")

  set(ICON_PREFIX_WITH_DESTDIR $ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/${ICON_PREFIX})
  install(FILES    ${CADABRA_IMAGES_DIR}/64x64/cadabra2-gtk.png     DESTINATION ${ICON_PREFIX}/share/icons/hicolor/64x64/apps)
  install(FILES    ${CADABRA_IMAGES_DIR}/128x128/cadabra2-gtk.png   DESTINATION ${ICON_PREFIX}/share/icons/hicolor/128x128/apps)
  install(FILES    ${CADABRA_IMAGES_DIR}/256x256/cadabra2-gtk.png   DESTINATION ${ICON_PREFIX}/share/icons/hicolor/256x256/apps)
  install(FILES    ${CADABRA_IMAGES_DIR}/cadabra2-gtk.svg           DESTINATION ${ICON_PREFIX}/share/icons/hicolor/scalable/apps)
  install(FILES    ${CADABRA_ROOT_DIR}/config/science.cadabra.cadabra2-gtk.desktop  DESTINATION ${ICON_PREFIX}/share/applications)
  if(NOT WIN32)
    # FIXME: Can this go on all platforms?
    install(CODE     "execute_process(COMMAND gtk-update-icon-cache --ignore-theme-index ${ICON_PREFIX_WITH_DESTDIR}/share/icons/hicolor)")
    install(CODE     "execute_process(COMMAND update-desktop-database ${ICON_PREFIX_WITH_DESTDIR}/share/applications)")
    install(CODE     "execute_process(COMMAND chmod go+r ${ICON_PREFIX_WITH_DESTDIR}/share/icons/hicolor/icon-theme.cache)")
  endif()
  install(DIRECTORY cdb-icons  DESTINATION share/cadabra2/)
  install(FILES     ${CMAKE_SOURCE_DIR}/config/cadabra2.ico  DESTINATION share/cadabra2/cdb-icons/)    

  # Install shared libraries on Windows. 
  # https://stackoverflow.com/questions/32662215/how-do-you-install-gtk-3-0-on-windows
  if(WIN32)
    # All things to make GTK apps work and look decent on Windows 10.
    # We always need the Adwaita theme for fallback icons, but VCPKG
    # does not yet distribute it, so we include it here (also see
    # https://github.com/Microsoft/vcpkg/pull/693). The Windows 10 theme
    # comes from https://github.com/B00merang-Project/Windows-10.
    # Settings as suggested by Luca Bacci on gtk-devel-list.
    install(FILES     config/gschemas.compiled DESTINATION share/glib-2.0/schemas)
    install(FILES     config/settings.ini      DESTINATION etc/gtk-3.0/)  
    install(DIRECTORY theme/Windows10          DESTINATION share/themes)
    install(DIRECTORY icons/Adwaita            DESTINATION share/icons)
    install(DIRECTORY icons/hicolor            DESTINATION share/icons)
    install(CODE [[
        execute_process(COMMAND cygpath -m "/ucrt64/lib/gdk-pixbuf-2.0" OUTPUT_VARIABLE WINGDK OUTPUT_STRIP_TRAILING_WHITESPACE)
        file(INSTALL ${WINGDK} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
    ]])
    install(CODE [[
        execute_process(COMMAND cygpath -m "/ucrt64/share/glib-2.0" OUTPUT_VARIABLE WINGLIB OUTPUT_STRIP_TRAILING_WHITESPACE)
        file(INSTALL ${WINGLIB} DESTINATION ${CMAKE_INSTALL_PREFIX}/share)
    ]])

    # Logic to get a start menu icon.
    # https://crascit.com/2015/08/07/cmake_cpack_nsis_shortcuts_with_parameters/
    # Does not work.
    set(CPACK_NSIS_CREATE_ICONS_EXTRA
      "CreateShortCut '$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\${CMAKE_PROJECT_NAME}.lnk' '$INSTDIR\\\\cadabra2-gtk.exe'")
    set(CPACK_NSIS_DELETE_ICONS_EXTRA
      "Delete '$SMPROGRAMS\\\\$START_MENU\\\\${CMAKE_PROJECT_NAME}.lnk'")

    set(CPACK_PACKAGE_EXECUTABLES "cadabra2-gtk" "Cadabra2")
    set(CPACK_NSIS_MENU_LINKS "cadabra2-gtk" "Cadabra2.lnk")
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

if(WIN32)
  install(DIRECTORY ../../examples/ DESTINATION Examples)
endif()

# microtex resources
if(USE_MICROTEX)
  add_dependencies(cadabra2-gtk microtex-cairo)
  install(DIRECTORY    ${PROJECT_SOURCE_DIR}/../../libs/cm DESTINATION ${INSTALL_MICROTEX_DIR})  
  install(DIRECTORY    ${microtex_BINARY_DIR}/res/lm-math DESTINATION ${INSTALL_MICROTEX_DIR})  
  install(FILES        ${PROJECT_SOURCE_DIR}/science.cadabra.cadabra2-gtk.appdata.xml DESTINATION share/metainfo/)
endif()

