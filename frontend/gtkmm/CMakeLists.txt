cmake_minimum_required(VERSION 2.8)
project(CadabraGtkmm)
message("-- Configuring gtkmm frontend")

if(CMAKE_COMPILER_IS_GNUCXX)
  add_definitions("-Wall -g -std=c++11 -Wno-unused-but-set-variable -D_WEBSOCKETPP_CPP11_STL_")
endif()
add_definitions("-DBOOST_LOG_DYN_LINK")

# Gtkmm-3.0 dependencies. 
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTKMM3 REQUIRED gtkmm-3.0)
include_directories(${GTKMM3_INCLUDE_DIRS})
link_directories(${GTKMM3_LIBRARY_DIRS})
add_definitions(${GTKMM3_CFLAGS_OTHER})
find_package(Boost 1.45.0 COMPONENTS system)
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
include_directories("${Boost_INCLUDE_DIRS}")
#pkg_check_modules(UUID uuid REQUIRED)
#link_directories(${UUID_LIBRARY_DIRS})

include_directories("." "../../core" "../../client_server" "../../client_server/jsoncpp" "../../client_server/websocketpp")

add_executable(cadabra2-gtk main.cc 
  NotebookWindow.cc 
  NotebookCanvas.cc 
  TeXView.cc 
  CodeInput.cc 
  ImageView.cc
  Cadabra.cc
  ../common/TeXEngine.cc)

# On Windows we need to link statically in order to avoid having to copy dll's 
# along with our binary. 
if(WIN32)
  target_link_libraries(cadabra2-gtk -static-libstdc++ -static-libgcc)
  target_link_libraries(cadabra2-gtk cadabra_client texengine ${Boost_LIBRARIES} ${GTKMM3_LIBRARIES} -lpthread)
else()
  target_link_libraries(cadabra2-gtk cadabra_client texengine ${Boost_LIBRARIES} ${GTKMM3_LIBRARIES} -lpthread)
endif()



# Create installation directories with correct permissions.
set(INSTALL_SHARE_DIR     share/cadabra2         CACHE PATH "Installation directory for shared files.")
set(INSTALL_IMAGES_DIR    share/cadabra2/images  CACHE PATH "Installation directory for image files.")
set(INSTALL_DESKTOP_DIR   share/applications     CACHE PATH "Installation directory for .desktop files.")
set(INSTALL_ICONS_DIR     share/icons            CACHE PATH "Installation directory for icons.")

install(DIRECTORY DESTINATION ${INSTALL_SHARE_DIR} DIRECTORY_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE 
                                                             GROUP_READ GROUP_EXECUTE 
                                                             WORLD_READ WORLD_EXECUTE)
install(DIRECTORY DESTINATION ${INSTALL_IMAGES_DIR} DIRECTORY_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE 
                                                             GROUP_READ GROUP_EXECUTE 
                                                             WORLD_READ WORLD_EXECUTE)
install(DIRECTORY DESTINATION ${INSTALL_DESKTOP_DIR} DIRECTORY_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE 
                                                             GROUP_READ GROUP_EXECUTE 
                                                             WORLD_READ WORLD_EXECUTE)
install(DIRECTORY DESTINATION ${INSTALL_ICONS_DIR} DIRECTORY_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE 
                                                             GROUP_READ GROUP_EXECUTE 
                                                             WORLD_READ WORLD_EXECUTE)
install(DIRECTORY DESTINATION ${INSTALL_ICONS_DIR}/hicolor DIRECTORY_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE 
                                                             GROUP_READ GROUP_EXECUTE 
                                                             WORLD_READ WORLD_EXECUTE)
install(DIRECTORY DESTINATION ${INSTALL_ICONS_DIR}/hicolor/64x64 DIRECTORY_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE 
                                                             GROUP_READ GROUP_EXECUTE 
                                                             WORLD_READ WORLD_EXECUTE)
install(DIRECTORY DESTINATION ${INSTALL_ICONS_DIR}/hicolor/256x256 DIRECTORY_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE 
                                                             GROUP_READ GROUP_EXECUTE 
                                                             WORLD_READ WORLD_EXECUTE)
install(DIRECTORY DESTINATION ${INSTALL_ICONS_DIR}/hicolor/scalable DIRECTORY_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE 
                                                             GROUP_READ GROUP_EXECUTE 
                                                             WORLD_READ WORLD_EXECUTE)
install(DIRECTORY DESTINATION ${INSTALL_ICONS_DIR}/hicolor/64x64/apps DIRECTORY_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE 
                                                             GROUP_READ GROUP_EXECUTE 
                                                             WORLD_READ WORLD_EXECUTE)
install(DIRECTORY DESTINATION ${INSTALL_ICONS_DIR}/hicolor/256x256/apps DIRECTORY_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE 
                                                             GROUP_READ GROUP_EXECUTE 
                                                             WORLD_READ WORLD_EXECUTE)
install(DIRECTORY DESTINATION ${INSTALL_ICONS_DIR}/hicolor/scalable/apps DIRECTORY_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE 
                                                             GROUP_READ GROUP_EXECUTE 
                                                             WORLD_READ WORLD_EXECUTE)


# Remove old versions of files which we no longer need, to avoid confusion.
install(CODE     "execute_process(COMMAND rm -f ${CMAKE_INSTALL_PREFIX}/bin/cadabra-gtk)")
install(CODE     "execute_process(COMMAND rm -f ${CMAKE_INSTALL_PREFIX}/bin/cadabra-qt5)")
install(CODE     "execute_process(COMMAND rm -f ${CMAKE_INSTALL_PREFIX}/share/applications/cadabra2.desktop)")
install(CODE     "execute_process(COMMAND rm -f ${CMAKE_INSTALL_PREFIX}/share/applications/cadabra2-gtk.desktop)")
install(CODE     "execute_process(COMMAND rm -f ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/64x64/apps/cadabra2.png)")
install(CODE     "execute_process(COMMAND rm -f ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/128x128/apps/cadabra2.png)")
install(CODE     "execute_process(COMMAND rm -f ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/256x256/apps/cadabra2.png)")
install(CODE     "execute_process(COMMAND rm -f ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/scalable/apps/cadabra2.svg)")
install(CODE     "execute_process(COMMAND rm -f ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/64x64/apps/cadabra2-gtk.png)")
install(CODE     "execute_process(COMMAND rm -f ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/128x128/apps/cadabra2-gtk.png)")
install(CODE     "execute_process(COMMAND rm -f ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/256x256/apps/cadabra2-gtk.png)")
install(CODE     "execute_process(COMMAND rm -f ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/scalable/apps/cadabra2-gtk.svg)")

# Install programs.
#find_program(XDG-DESKTOP-MENU_EXECUTABLE xdg-desktop-menu REQUIRED)
install(TARGETS  cadabra2-gtk     DESTINATION bin)
install(FILES    ${PROJECT_SOURCE_DIR}/../../images/256x256/cadabra2-gtk.png DESTINATION ${INSTALL_IMAGES_DIR})
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

if(APPLE)
  set(ICON_PREFIX "${CMAKE_INSTALL_PREFIX}")
else()
  set(ICON_PREFIX "/usr")
endif()

#set(ICON_THEME ${ICON_PREFIX}/share/icons/hicolor/index.theme)
#if(EXISTS "${ICON_THEME}")
#   message("-- ${ICON_THEME} exists, good")
#else()
#   message("-- Will copy index.theme from /usr/share/icons/hicolor/index.theme")
#	install(CODE     "execute_process(COMMAND cp /usr/share/icons/hicolor/index.theme ${ICON_THEME})")
#endif()
install(FILES    ${PROJECT_SOURCE_DIR}/../../images/64x64/cadabra2-gtk.png     DESTINATION ${ICON_PREFIX}/share/icons/hicolor/64x64/apps)
install(FILES    ${PROJECT_SOURCE_DIR}/../../images/128x128/cadabra2-gtk.png   DESTINATION ${ICON_PREFIX}/share/icons/hicolor/128x128/apps)
install(FILES    ${PROJECT_SOURCE_DIR}/../../images/256x256/cadabra2-gtk.png   DESTINATION ${ICON_PREFIX}/share/icons/hicolor/256x256/apps)
install(FILES    ${PROJECT_SOURCE_DIR}/../../images/cadabra2-gtk.svg           DESTINATION ${ICON_PREFIX}/share/icons/hicolor/scalable/apps)
install(FILES    ${PROJECT_SOURCE_DIR}/../../config/cadabra2-gtk.desktop       DESTINATION ${ICON_PREFIX}/share/applications)
install(CODE     "execute_process(COMMAND touch ${ICON_PREFIX}/share/icons/hicolor/icon-theme.cache)")
install(CODE     "execute_process(COMMAND gtk-update-icon-cache-3.0 ${ICON_PREFIX}/share/icons/hicolor)")
install(CODE     "execute_process(COMMAND chmod go+r ${ICON_PREFIX}/share/icons/hicolor/icon-theme.cache)")
