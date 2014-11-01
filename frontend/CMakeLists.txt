cmake_minimum_required(VERSION 2.8)

project(Cadabra)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTKMM3 OPTIONAL gtkmm-3.0)
subdirs(common)
if(GTKMM3_FOUND)
   subdirs(gtkmm)
   message("-- Building GTK3 user interface")
endif(GTKMM3_FOUND)

if(APPLE)
   subdirs(osx)
   message("-- Building OSX native interface")
endif(APPLE)
