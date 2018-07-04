# Collection of utilities for using
# vcpkg to find libraries in CMake

list(LENGTH CMAKE_CONFIGURATION_TYPES N_CONFIGURATION_TYPES)
if (${N_CONFIGURATION_TYPES} EQUAL 1)
	set(CMAKE_BUILD_TYPE ${CMAKE_CONFIGURATION_TYPES})
endif()

# Attempts to find the vcpkg.cmake toolchain file and include it
# if it has not been supplied.
if (NOT VCPKG_TOOLCHAIN)
	find_file(CMAKE_TOOLCHAIN_FILE vcpkg.cmake HINTS $ENV{userprofile} $ENV{systemdrive} PATH_SUFFIXES vcpkg/scripts/buildsystems)
	if (CMAKE_TOOLCHAIN_FILE)
		include(${CMAKE_TOOLCHAIN_FILE})
	endif()
endif()
if (VCPKG_TOOLCHAIN)
  message(STATUS "Found vcpkg at ${_VCPKG_ROOT_DIR}")
else()
  message(FATAL_ERROR "Cold not find vcpkg (required for building on Visual Studio)")
endif()

if (VCPKG_TOOLCHAIN)
	# Location of include files
	set(VCPKG_INCLUDE_DIRS ${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include)

	if(CMAKE_BUILD_TYPE MATCHES "^Debug$" OR NOT DEFINED CMAKE_BUILD_TYPE)
		set(VCPKG_LIB_DIRS 
			${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/debug/lib
			${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/debug/lib/manual-link
			${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib
			${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib/manual-link
		)
		set(VCPKG_BIN_DIRS 
			${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/debug/bin
			${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin
		)
	else()
		set(VCPKG_LIB_DIRS 
			${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib
			${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib/manual-link
		)
		set(VCPKG_BIN_DIRS 
			${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin
		)
	endif()

	function(windows_find_file VAR FNAME FEXT)
		set(TMPVAR "")
		foreach (DIR ${VCPKG_LIB_DIRS})
			# If a debug build is specified, first try and find the library name
			# with a debug marker
			if(CMAKE_BUILD_TYPE MATCHES "^Debug$" OR NOT DEFINED CMAKE_BUILD_TYPE)
				if ("${TMPVAR}" STREQUAL "") # name-d.lib
					file(GLOB TMPVAR "${DIR}/${FNAME}-d.${FEXT}")
				endif()
				if ("${TMPVAR}" STREQUAL "") # named.lib
					file(GLOB TMPVAR "${DIR}/${FNAME}d.${FEXT}")
				endif()
				if ("${TMPVAR}" STREQUAL "") # name-<version-number>-d.lib
					file(GLOB TMPVAR "${DIR}/${FNAME}-[0-9]*-d.${FEXT}")
				endif()
				if ("${TMPVAR}" STREQUAL "") # name-<version-number>d.lib
					file(GLOB TMPVAR "${DIR}/${FNAME}-[0-9]*-d.${FEXT}")
				endif()
				if ("${TMPVAR}" STREQUAL "") # name<version-number>-d.lib
					file(GLOB TMPVAR "${DIR}/${FNAME}[0-9]*-d.${FEXT}")
				endif()
				if ("${TMPVAR}" STREQUAL "") # name<version-number>d.lib
					file(GLOB TMPVAR "${DIR}/${FNAME}[0-9]*d.${FEXT}")
				endif()
				if ("${TMPVAR}" STREQUAL "") # name-d-<version-number>.lib
					file(GLOB TMPVAR "${DIR}/${FNAME}-d-[0-9]*.${FEXT}")
				endif()
				if ("${TMPVAR}" STREQUAL "") # named-<version-number>.lib
					file(GLOB TMPVAR "${DIR}/${FNAME}d-[0-9]*.${FEXT}")
				endif()
				if ("${TMPVAR}" STREQUAL "") # name-d<version-number>.lib
					file(GLOB TMPVAR "${DIR}/${FNAME}-d[0-9]*.${FEXT}")
				endif()
				if ("${TMPVAR}" STREQUAL "") # named<version-number>.lib
					file(GLOB TMPVAR "${DIR}/${FNAME}d[0-9]*.${FEXT}")
				endif()
			endif()
			# Attempt to find library name without debug marker
			if ("${TMPVAR}" STREQUAL "") # name.lib
				file(GLOB TMPVAR "${DIR}/${FNAME}.${FEXT}")
			endif()
			if ("${TMPVAR}" STREQUAL "") # name-<version-number>.lib
				file(GLOB TMPVAR "${DIR}/${FNAME}-[0-9]*.${FEXT}")
			endif()
			if ("${TMPVAR}" STREQUAL "") # name<version-number>.lib
				file(GLOB TMPVAR "${DIR}/${FNAME}[0-9]*.${FEXT}")
			endif()
		endforeach()
		# Assign the result of the search to VAR
		if ("${TMPVAR}" STREQUAL "")
			# Couldn't find it, set to NOTFOUND
			set(${VAR} "${VAR}-NOTFOUND" PARENT_SCOPE)
		else()
			# GLOB could return a list of matching filenames, in which case
			# we choose the latest version (i.e. the one which is alphabetically
			# last)
			list(SORT TMPVAR)
			list(REVERSE TMPVAR)
			list(GET TMPVAR 0 TMPVAR)
			set(${VAR} "${TMPVAR}")
		endif()
	endfunction()

	function(windows_find_library VAR)
		# Avoid some horrible indirections by using a local variable
		set(OUTVAR "${${VAR}}")

		# If the variable is already specified, don't attempt to 
		# find it again
		if ("${OUTVAR}" STREQUAL "")
		else()
			return()
		endif()

		# Attempt to find the REQUIRED flag
		foreach(FLAG ${ARGN})
			if (FLAG STREQUAL "REQUIRED")
				set(IS_REQUIRED TRUE)
			endif()
		endforeach()

		# Loop over all the library names
		foreach(LIBNAME ${ARGN})
			set(TMPVAR "")
			# Could be the required flag
			if (LIBNAME STREQUAL "REQUIRED")
				continue()
			endif()
			# Loop over all library directories, searching for the
			# library name
			foreach (DIR ${VCPKG_LIB_DIRS})
				# If a debug build is specified, first try and find the library name
				# with a debug marker
				if(CMAKE_BUILD_TYPE MATCHES "^Debug$" OR NOT DEFINED CMAKE_BUILD_TYPE)
					if ("${TMPVAR}" STREQUAL "") # name-d.lib
						file(GLOB TMPVAR "${DIR}/${LIBNAME}-d.lib")
					endif()
					if ("${TMPVAR}" STREQUAL "") # named.lib
						file(GLOB TMPVAR "${DIR}/${LIBNAME}d.lib")
					endif()
					if ("${TMPVAR}" STREQUAL "") # name-<version-number>-d.lib
						file(GLOB TMPVAR "${DIR}/${LIBNAME}-[0-9]*-d.lib")
					endif()
					if ("${TMPVAR}" STREQUAL "") # name-<version-number>d.lib
						file(GLOB TMPVAR "${DIR}/${LIBNAME}-[0-9]*-d.lib")
					endif()
					if ("${TMPVAR}" STREQUAL "") # name<version-number>-d.lib
						file(GLOB TMPVAR "${DIR}/${LIBNAME}[0-9]*-d.lib")
					endif()
					if ("${TMPVAR}" STREQUAL "") # name<version-number>d.lib
						file(GLOB TMPVAR "${DIR}/${LIBNAME}[0-9]*d.lib")
					endif()
					if ("${TMPVAR}" STREQUAL "") # name-d-<version-number>.lib
						file(GLOB TMPVAR "${DIR}/${LIBNAME}-d-[0-9]*.lib")
					endif()
					if ("${TMPVAR}" STREQUAL "") # named-<version-number>.lib
						file(GLOB TMPVAR "${DIR}/${LIBNAME}d-[0-9]*.lib")
					endif()
					if ("${TMPVAR}" STREQUAL "") # name-d<version-number>.lib
						file(GLOB TMPVAR "${DIR}/${LIBNAME}-d[0-9]*.lib")
					endif()
					if ("${TMPVAR}" STREQUAL "") # named<version-number>.lib
						file(GLOB TMPVAR "${DIR}/${LIBNAME}d[0-9]*.lib")
					endif()
				endif()
				# Attempt to find library name without debug marker
				if ("${TMPVAR}" STREQUAL "") # name.lib
					file(GLOB TMPVAR "${DIR}/${LIBNAME}.lib")
				endif()
				if ("${TMPVAR}" STREQUAL "") # name-<version-number>.lib
					file(GLOB TMPVAR "${DIR}/${LIBNAME}-[0-9]*.lib")
				endif()
				if ("${TMPVAR}" STREQUAL "") # name<version-number>.lib
					file(GLOB TMPVAR "${DIR}/${LIBNAME}[0-9]*.lib")
				endif()
			endforeach()
			# Assign the result of the search to VAR
			if ("${TMPVAR}" STREQUAL "")
				# Couldn't find it, set to NOTFOUND
				set(${VAR} "${VAR}-NOTFOUND" PARENT_SCOPE)
				if (IS_REQUIRED)
					message(FATAL_ERROR "Could NOT find REQUIRED library ${LIBNAME} required for ${VAR}")
				else()
					message(SEND_ERROR "Could NOT find library ${LIBNAME} required for ${VAR}")
				endif()
			else()
				# GLOB could return a list of matching filenames, in which case
				# we choose the latest version (i.e. the one which is alphabetically
				# last)
				list(SORT TMPVAR)
				list(REVERSE TMPVAR)
				list(GET TMPVAR 0 TMPVAR)
				list(APPEND OUTVAR "${TMPVAR}")
			endif()
		endforeach()
		set(${VAR} "${OUTVAR}" PARENT_SCOPE)
	endfunction()
endif()