cmake_minimum_required(VERSION 2.8)

project(Cadabra)
set(CADABRA_VERSION_MAJOR 2)
set(CADABRA_VERSION_MINOR 0)
set(COPYRIGHT_YEARS "2001-2016")
execute_process(COMMAND git rev-parse --short HEAD OUTPUT_VARIABLE GIT_SHORT_SHA     OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND git rev-list --count HEAD  OUTPUT_VARIABLE GIT_COMMIT_SERIAL OUTPUT_STRIP_TRAILING_WHITESPACE)
set(CADABRA_VERSION_BUILD "${GIT_COMMIT_SERIAL}.${GIT_SHORT_SHA}")
message("-- Build id ${CADABRA_VERSION_BUILD}")

enable_testing()

# Generate config file with CMake variables.
configure_file("${PROJECT_SOURCE_DIR}/core/Config.hh.in" "${PROJECT_BINARY_DIR}/core/Config.hh")
include_directories(${PROJECT_BINARY_DIR}/core)

# Determine what type of system we are on, for packaging.
if(EXISTS "/etc/issue")
   set(LINUX_NAME "")
   file(READ "/etc/issue" LINUX_ISSUE)
   if(LINUX_ISSUE MATCHES "Fedora")
     set(CPACK_GENERATOR "RPM")      
	  message("-- This is a Fedora system")
   endif(LINUX_ISSUE MATCHES "Fedora")
   if(LINUX_ISSUE MATCHES "Ubuntu")
     set(CPACK_GENERATOR "DEB")
	  message("-- This is an Ubuntu system")
   endif(LINUX_ISSUE MATCHES "Ubuntu")
   if(LINUX_ISSUE MATCHES "Mint")
     set(CPACK_GENERATOR "DEB")
	  message("-- This is a Linux Mint system")
   endif(LINUX_ISSUE MATCHES "Mint")
endif(EXISTS "/etc/issue")

# Logic to build packages (RPM/DEB) using CPack.
include (InstallRequiredSystemLibraries)
set(CPACK_PACKAGE_NAME           "cadabra2")
set(CPACK_RESOURCE_FILE_LICENSE  "${CMAKE_CURRENT_SOURCE_DIR}/doc/license.txt")
set(CPACK_PACKAGE_VERSION_MAJOR  "${CADABRA_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR  "${CADABRA_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH  "${CADABRA_VERSION_BUILD}")
set(CPACK_PACKAGE_CONTACT        "Kasper Peeters <kasper.peeters@phi-sci.com>")
set(CPACK_SYSTEM_NAME            "trusty")
set(CPACK_STRIP_FILES            ON)
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA texhash)

set(CPACK_DEBIAN_PACKAGE_DEPENDS "libpcre3, libpcrecpp0, libgmpxx4dbl, libboost-python1.54, libboost-system1.54, libstdc++6, uuid-runtime, libgtkmm-3.0, texlive, texlive-latex-extra, python-matplotlib, python-mpmath, dvipng, python-sympy")
set(CPACK_DEBIAN_PACKAGE_SECTION "math")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "field-theory motivated computer algebra system")
set(CPACK_PACKAGE_DESCRIPTION "Cadabra is a computer algebra system designed specifically for the
 solution of problems encountered in field theory. It has extensive
 functionality for tensor polynomial simplification including
 multi-term symmetries, fermions and anti-commuting variables,
 Clifford algebras and Fierz transformations, implicit coordinate
 dependence, multiple index types and many more. The input format is
 a subset of TeX. This is the 2.x series of the software.")
include(CPack)

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O2")
endif()

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O2")
# -I/usr/local/Cellar/python/2.7.8_2/Frameworks/Python.framework/Versions/2.7/include/python2.7")
endif()


# Set path to additional cmake files
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules")

add_subdirectory(core)
add_subdirectory(client_server)
add_subdirectory(frontend)
add_subdirectory(tests)
add_subdirectory(web2 EXCLUDE_FROM_ALL)

