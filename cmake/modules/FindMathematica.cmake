# - Try to find Mathematica installation and provide CMake functions for its C/C++ interface
#
# See the FindMathematica manual for usage hints.
#
#=============================================================================
# Copyright 2010-2017 Sascha Kratky
#
# Permission is hereby granted, free of charge, to any person)
# obtaining a copy of this software and associated documentation)
# files (the "Software"), to deal in the Software without)
# restriction, including without limitation the rights to use,)
# copy, modify, merge, publish, distribute, sublicense, and/or sell)
# copies of the Software, and to permit persons to whom the)
# Software is furnished to do so, subject to the following)
# conditions:)
#
# The above copyright notice and this permission notice shall be)
# included in all copies or substantial portions of the Software.)
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,)
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES)
# OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND)
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT)
# HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,)
# WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING)
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR)
# OTHER DEALINGS IN THE SOFTWARE.)
#=============================================================================

# we need the CMakeParseArguments module
# call cmake_minimum_required, but prevent modification of the CMake policy stack
cmake_policy(PUSH)
cmake_minimum_required(VERSION 2.8.12)
cmake_policy(POP)

set (Mathematica_CMAKE_MODULE_DIR "${CMAKE_CURRENT_LIST_DIR}")
set (Mathematica_CMAKE_MODULE_VERSION "3.2.3")

# activate select policies
if (POLICY CMP0025)
	# Compiler id for Apple Clang is now AppleClang
	cmake_policy(SET CMP0025 NEW)
endif()

if (POLICY CMP0026)
	# disallow use of the LOCATION target property
	if (CYGWIN OR MSYS)
		# Cygwin and MSYS do not produce workable Mathematica paths using
		# the $<TARGET_FILE:...> notation
		cmake_policy(SET CMP0026 OLD)
	else()
		cmake_policy(SET CMP0026 NEW)
	endif()
endif()

if (POLICY CMP0038)
	# targets may not link directly to themselves
	cmake_policy(SET CMP0038 NEW)
endif()

if (POLICY CMP0039)
	# utility targets may not have link dependencies
	cmake_policy(SET CMP0039 NEW)
endif()

if (POLICY CMP0040)
	# target in the TARGET signature of add_custom_command() must exist
	cmake_policy(SET CMP0040 NEW)
endif()

if (POLICY CMP0045)
	# error on non-existent target in get_target_property
	cmake_policy(SET CMP0045 NEW)
endif()

if (POLICY CMP0046)
	# error on non-existent dependency in add_dependencies
	cmake_policy(SET CMP0046 NEW)
endif()

if (POLICY CMP0049)
	# do not expand variables in target source entries
	cmake_policy(SET CMP0049 NEW)
endif()

if (POLICY CMP0050)
	# disallow add_custom_command SOURCE signatures
	cmake_policy(SET CMP0050 NEW)
endif()

if (POLICY CMP0051)
	# include TARGET_OBJECTS expressions in a target's SOURCES property
	cmake_policy(SET CMP0051 NEW)
endif()

if (POLICY CMP0053)
	# simplify variable reference and escape sequence evaluation
	cmake_policy(SET CMP0053 NEW)
endif()

if (POLICY CMP0054)
	# only interpret if() arguments as variables or keywords when unquoted
	cmake_policy(SET CMP0054 NEW)
endif()

include(TestBigEndian)
include(CMakeParseArguments)
include(FindPackageHandleStandardArgs)
include(CMakeFindFrameworks)

# internal function to convert Windows path to Cygwin workable CMake path
# E.g., "C:\Program Files" is converted to "/cygdrive/c/Program Files"
# file(TO_CMAKE_PATH "C:\Program Files" ...) would result in unworkable "C;/Program Files"
function (_to_cmake_path _inPath _outPathVariable)
	if (CYGWIN)
		find_program(Mathematica_CYGPATH_EXECUTABLE "cygpath")
		mark_as_advanced(Mathematica_CYGPATH_EXECUTABLE)
		execute_process(
			COMMAND "${Mathematica_CYGPATH_EXECUTABLE}" "--unix" "${_inPath}" TIMEOUT 5
			OUTPUT_VARIABLE ${_outPathVariable} OUTPUT_STRIP_TRAILING_WHITESPACE)
	else()
		file(TO_CMAKE_PATH "${_inPath}" ${_outPathVariable})
	endif()
	set (${_outPathVariable} "${${_outPathVariable}}" PARENT_SCOPE)
endfunction()

# internal function to convert CMake path to "pure" native path without escapes
function (_to_native_path _inPath _outPathVariable)
	# do not use the built-in function file (TO_NATIVE_PATH ...),
	# which does too much or the wrong thing:
	# it converts a CMake path to a native path but then also escapes all blanks
	# and special characters
	# under MinGW it produces unworkable paths with forward slashes
	if (CYGWIN)
		find_program(Mathematica_CYGPATH_EXECUTABLE "cygpath")
		mark_as_advanced(Mathematica_CYGPATH_EXECUTABLE)
		execute_process(
			COMMAND "${Mathematica_CYGPATH_EXECUTABLE}" "--mixed" "${_inPath}" TIMEOUT 5
			OUTPUT_VARIABLE ${_outPathVariable} OUTPUT_STRIP_TRAILING_WHITESPACE)
	elseif (CMAKE_HOST_UNIX)
		# use CMake path literally under UNIX
		set (${_outPathVariable} "${_inPath}")
	elseif (CMAKE_HOST_WIN32)
		string (REPLACE "/" "\\" ${_outPathVariable} "${_inPath}")
	else()
		message (FATAL_ERROR "Unsupported host platform ${CMAKE_HOST_SYSTEM_NAME}")
	endif()
	set (${_outPathVariable} "${${_outPathVariable}}" PARENT_SCOPE)
endfunction()

# internal macro to set a file's executable bit under UNIX
macro (_make_file_executable _inPath)
	if (CMAKE_HOST_UNIX)
		_to_native_path ("${_inPath}" _nativePath)
		execute_process(
			COMMAND chmod "-f" "+x" "${_nativePath}" TIMEOUT 5)
	endif()
endmacro()

# internal macro to convert list to command string with quoting
macro (_list_to_cmd_str _outCmd)
	set (_str "")
	foreach (_arg ${ARGN})
		if ("${_arg}" MATCHES " ")
			set (_arg "\"${_arg}\"")
		endif()
		if (_str)
			set (_str "${_str} ${_arg}")
		else()
			set (_str "${_arg}")
		endif()
	endforeach()
	set (${_outCmd} "${_str}")
endmacro()

# internal macro to compute kernel paths (relative to installation directory)
macro (_get_host_kernel_names _outKernelNames)
	if (Mathematica_FIND_VERSION AND Mathematica_FIND_VERSION_EXACT)
		if (Mathematica_FIND_VERSION VERSION_LESS "10.0.0")
			if (CMAKE_HOST_WIN32 OR CYGWIN)
				set (${_outKernelNames} "math.exe")
			elseif (CMAKE_HOST_APPLE)
				set (${_outKernelNames} "Contents/MacOS/MathKernel")
			elseif (CMAKE_HOST_UNIX)
				set (${_outKernelNames} "Executables/MathKernel" "Executables/math")
			endif()
		else()
			if (CMAKE_HOST_WIN32 OR CYGWIN)
				set (${_outKernelNames} "wolfram.exe")
			elseif (CMAKE_HOST_APPLE)
				set (${_outKernelNames} "Contents/MacOS/WolframKernel")
			elseif (CMAKE_HOST_UNIX)
				set (${_outKernelNames} "Executables/WolframKernel")
			endif()
		endif()
	else()
		if (CMAKE_HOST_WIN32 OR CYGWIN)
			set (${_outKernelNames} "wolfram.exe" "math.exe")
		elseif (CMAKE_HOST_APPLE)
			set (${_outKernelNames} "Contents/MacOS/WolframKernel" "Contents/MacOS/MathKernel")
		elseif (CMAKE_HOST_UNIX)
			set (${_outKernelNames} "Executables/WolframKernel" "Executables/MathKernel" "Executables/math")
		endif()
	endif()
endmacro()

# internal macro to to compute front end paths (relative to installation directory)
macro (_get_host_frontend_names _outFrontEndNames)
	if (CMAKE_HOST_WIN32 OR CYGWIN)
		set (${_outFrontEndNames} "Mathematica.exe")
	elseif (CMAKE_HOST_APPLE)
		set (${_outFrontEndNames} "Contents/MacOS/Mathematica")
	elseif (CMAKE_HOST_UNIX)
		set (${_outFrontEndNames}
			"Executables/mathematica" "Executables/Mathematica")
	endif()
endmacro()

# internal macro to compute program name from product name and version
# E.g., "Mathematica" and "7.0" gives "Mathematica 7.0.app" for Mac OS X
macro (_append_program_names _product _version _outProgramNames)
	string (REPLACE " " "" _productWithoutBlanks "${_product}")
	if (CMAKE_HOST_APPLE)
		if (${_version})
			# under Mac OS X the application name may contain the version number as a suffix
			list (APPEND ${_outProgramNames} "${_product} ${_version}.app")
			list (APPEND ${_outProgramNames} "${_productWithoutBlanks} ${_version}.app")
		else()
			list (APPEND ${_outProgramNames} "${_product}.app")
			list (APPEND ${_outProgramNames} "${_productWithoutBlanks}.app")
		endif()
	else()
		if (${_version})
			# other platforms have a sub-directory named after the version number
			list (APPEND ${_outProgramNames} "${_product}/${_version}")
			list (APPEND ${_outProgramNames} "${_productWithoutBlanks}/${_version}")
		endif()
	endif()
endmacro()

# internal macro to determine search order for different versions of Mathematica
macro (_get_program_names _outProgramNames)
	set (${_outProgramNames} "")
	# Mathematica products in order of preference
	set (_MathematicaApps "Mathematica" "gridMathematica Server")
	# Mathematica product versions in order of preference
	set (_MathematicaVersions "11.2" "11.1" "11.0" "10.4" "10.3" "10.2" "10.1" "10.0" "9.0" "8.0" "7.0" "6.0" "5.2")
	# search for explicitly requested application version first
	if (Mathematica_FIND_VERSION AND Mathematica_FIND_VERSION_EXACT)
		foreach (_product IN LISTS _MathematicaApps)
			_append_program_names("${_product}"
				"${Mathematica_FIND_VERSION_MAJOR}.${Mathematica_FIND_VERSION_MINOR}"
				${_outProgramNames})
		endforeach()
	endif()
	# then try all qualified application names
	foreach (_product IN LISTS _MathematicaApps)
		foreach (_version IN LISTS _MathematicaVersions)
			_append_program_names("${_product}" "${_version}" ${_outProgramNames})
		endforeach()
	endforeach()
	# then try unqualified application names
	foreach (_product IN LISTS _MathematicaApps)
		_append_program_names("${_product}" "" ${_outProgramNames})
	endforeach()
	list (REMOVE_DUPLICATES ${_outProgramNames})
endmacro()

# internal function to get Mathematica Windows installation directory for a registry entry
function (_add_registry_search_path _registryKey _outSearchPaths)
	set (_ProductNamePatterns "Wolfram Mathematica [0-9]+" "Wolfram Finance Platform")
	get_filename_component (
		_productName "[${_registryKey};ProductName]" NAME)
	get_filename_component (
		_productVersion "[${_registryKey};ProductVersion]" NAME)
	get_filename_component (
		_productPath "[${_registryKey};ExecutablePath]" PATH)
	if (Mathematica_DEBUG)
		message (STATUS "[${_registryKey};ProductName]=${_productName}")
		message (STATUS "[${_registryKey};ProductVersion]=${_productVersion}")
		message (STATUS "[${_registryKey};ExecutablePath]=${_productPath}")
	endif()
	set (_qualified False)
	foreach (_Pattern IN LISTS _ProductNamePatterns)
		if ("${_productName}" MATCHES "${_Pattern}")
			set (_qualified True)
			break()
		endif()
	endforeach()
	if (_qualified)
		if (EXISTS "${_productPath}")
			_to_cmake_path("${_productPath}" _path)
			if (Mathematica_FIND_VERSION AND Mathematica_FIND_VERSION_EXACT)
				if ("${_productVersion}" MATCHES "${Mathematica_FIND_VERSION}")
					# prepend if version matches requested one
					list (INSERT ${_outSearchPaths} 0 "${_path}")
				else()
					list (APPEND ${_outSearchPaths} "${_path}")
				endif()
			else()
				list (APPEND ${_outSearchPaths} "${_path}")
			endif()
		endif()
	endif()
	set (${_outSearchPaths} ${${_outSearchPaths}} PARENT_SCOPE)
endfunction()

# internal function to determine Mathematica installation paths from Windows registry
function (_add_registry_search_paths _outSearchPaths)
	if (CMAKE_HOST_WIN32)
		foreach (_registryKey IN ITEMS ${ARGN})
			set (_regExe "reg.exe")
			if (DEFINED ENV{windir})
				# use 64-bit reg.exe under WoW64 to make sure we search all keys
				if (EXISTS "$ENV{windir}/sysnative/reg.exe")
					set (_regExe "$ENV{windir}/sysnative/reg.exe")
				endif()
			endif()
			execute_process(
				COMMAND "${_regExe}" query "${_registryKey}" "/s"
				COMMAND findstr "${_registryKey}"
				TIMEOUT 5 OUTPUT_VARIABLE _queryResult ERROR_QUIET)
			string (REGEX MATCHALL "[0-9]+" _installIDs "${_queryResult}")
			if (_installIDs)
				# _installIDs sorted from oldest to newest version
				list (REVERSE _installIDs)
				set (_paths "")
				foreach (_installID IN LISTS _installIDs)
					_add_registry_search_path("${_registryKey}\\${_installID}" _paths)
				endforeach()
				list (APPEND ${_outSearchPaths} ${_paths})
			endif()
		endforeach()
		set (${_outSearchPaths} ${${_outSearchPaths}} PARENT_SCOPE)
	endif()
endfunction()

# internal function to determine Mathematica installation paths from Mac OS X LaunchServices database
function (_add_launch_services_search_paths _outSearchPaths)
	if (CMAKE_HOST_APPLE)
		# the lsregister executable is needed to search the LaunchServices database
		# the executable usually resides in the LaunchServices framework Support directory
		# The LaunchServices framework is a sub-framework of the CoreServices umbrella framework
		cmake_find_frameworks(CoreServices)
		find_program (Mathematica_LSRegister_EXECUTABLE NAMES "lsregister" PATH_SUFFIXES "Support"
			HINTS "${CoreServices_FRAMEWORKS}/Frameworks/LaunchServices.framework")
		mark_as_advanced(
			Mathematica_CoreServices_DIR
			Mathematica_LaunchServices_DIR
			Mathematica_LSRegister_EXECUTABLE)
		if (NOT Mathematica_LSRegister_EXECUTABLE)
			message (STATUS "Skipping search of the LaunchServices database, because the lsregister executable could not be found.")
			return()
		endif()
		foreach (_bundleID IN ITEMS ${ARGN})
			execute_process(
				COMMAND "${Mathematica_LSRegister_EXECUTABLE}" "-dump"
				COMMAND "grep" "--before-context=20" "--after-context=20" " ${_bundleID} "
				COMMAND "grep" "--only-matching" "/.*\\.app"
				TIMEOUT 10 OUTPUT_VARIABLE _queryResult ERROR_QUIET)
			string (REPLACE ";" "\\;" _queryResult "${_queryResult}")
			string (REPLACE "\n" ";" _appPaths "${_queryResult}")
			if (_appPaths)
				# put paths into canonical order
				list (SORT _appPaths)
				list (REVERSE _appPaths)
			else()
				message (STATUS "No Mathematica apps registered in Mac OS X LaunchServices database.")
			endif()
			if (Mathematica_DEBUG)
				message (STATUS "Mac OS X LaunchServices database registered apps=${_appPaths}")
			endif()
			if (_appPaths)
				set (_paths "")
				set (_insertIndex 0)
				foreach (_appPath IN LISTS _appPaths)
					# ignore paths that no longer exist
					if (EXISTS "${_appPath}")
						_to_cmake_path("${_appPath}" _appPath)
						if (Mathematica_FIND_VERSION AND Mathematica_FIND_VERSION_EXACT)
							if ("${_appPath}" MATCHES "${Mathematica_FIND_VERSION_MAJOR}.${Mathematica_FIND_VERSION_MINOR}")
								# insert in front of other versions if version matches requested one
								list (LENGTH _paths _len)
								if (_len EQUAL _insertIndex)
									list (APPEND _paths "${_appPath}")
								else()
									list (INSERT _paths ${_insertIndex} "${_appPath}")
								endif()
								math(EXPR _insertIndex "${_insertIndex} + 1")
							else()
								list (APPEND _paths "${_appPath}")
							endif()
						else()
							list (APPEND _paths "${_appPath}")
						endif()
					endif()
				endforeach()
				list (APPEND ${_outSearchPaths} ${_paths})
			endif()
		endforeach()
		set (${_outSearchPaths} ${${_outSearchPaths}} PARENT_SCOPE)
	endif()
endfunction()

# internal macro to determine default Mathematica installation (the one which is on the system search path)
macro (_add_default_search_path _outSearchPaths)
	set (_searchPaths "")
	if (DEFINED ENV{PATH})
		file (TO_CMAKE_PATH "$ENV{PATH}" _searchPaths)
	endif()
	_get_host_kernel_names(_kernelNames)
	foreach (_searchPath IN LISTS _searchPaths)
		if (CMAKE_HOST_WIN32 OR CYGWIN)
			set (_executable "${_searchPath}/math.exe")
		else()
			set (_executable "${_searchPath}/math")
		endif()
		if (EXISTS "${_executable}")
			# resolve symlinks
			get_filename_component (_executable "${_executable}" REALPATH)
			foreach (_kernelName IN LISTS _kernelNames)
				string (REPLACE "${_kernelName}" "" _executableDir "${_executable}")
				if (NOT "${_executable}" STREQUAL "${_executableDir}" AND
					IS_DIRECTORY ${_executableDir})
					if (Mathematica_FIND_VERSION)
						list (APPEND ${_outSearchPaths} "${_executableDir}")
					else()
						# prefer default installation if not searching for version explicitly
						list (INSERT ${_outSearchPaths} 0 "${_executableDir}")
					endif()
				endif()
			endforeach()
		endif()
	endforeach()
endmacro()

# internal macro to determine platform specific Mathematica installation search paths
macro (_get_search_paths _outSearchPaths)
	set (${_outSearchPaths} "")
	if (CMAKE_HOST_WIN32 OR CYGWIN)
		# add non-standard installation paths from Windows registry
		_add_registry_search_paths(${_outSearchPaths}
			"HKEY_LOCAL_MACHINE\\SOFTWARE\\Wolfram Research\\Installations"
			"HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\Wolfram Research\\Installations")
		# environment variable locations where Mathematica may be installed
		set (_WindowsProgramFilesEnvVars to "ProgramW6432" "ProgramFiles(x86)" "ProgramFiles" )
		if (CYGWIN)
			# Cygwin may be configured to convert all environment variables to all-uppercase
			list (APPEND _WindowsProgramFilesEnvVars "PROGRAMW6432" "PROGRAMFILES(X86)" "PROGRAMFILES")
		endif()
		# add standard Mathematica Windows installation paths
		foreach (_envVar IN LISTS _WindowsProgramFilesEnvVars)
			if (DEFINED ENV{${_envVar})
				_to_cmake_path("$ENV{${_envVar}}" _unixPath)
				list (APPEND ${_outSearchPaths} "${_unixPath}/Wolfram Research" )
			endif()
		endforeach()
	elseif (CMAKE_HOST_APPLE)
		# add standard Mathematica Mac OS X installation paths
		list (APPEND ${_outSearchPaths} "~/Applications;/Applications")
		if (CMAKE_SYSTEM_APPBUNDLE_PATH)
			list (APPEND ${_outSearchPaths} ${CMAKE_SYSTEM_APPBUNDLE_PATH})
		endif()
		# add non-standard installation paths from Mac OS X LaunchServices database
		_add_launch_services_search_paths(${_outSearchPaths} "com.wolfram.Mathematica")
	elseif (CMAKE_HOST_UNIX)
		# add standard Mathematica Unix installation paths
		list (APPEND ${_outSearchPaths} "/usr/local/Wolfram" "/opt/Wolfram")
	endif()
	_add_default_search_path(${_outSearchPaths})
	if (${_outSearchPaths})
		list (REMOVE_DUPLICATES ${_outSearchPaths})
	endif()
endmacro()

# internal macro to compute Mathematica SystemIDs from system name
macro (_systemNameToSystemID _systemName _systemProcessor _outSystemIDs)
	if ("${_systemName}" STREQUAL "Windows")
		if ("${_systemProcessor}" STREQUAL "AMD64")
			set (${_outSystemIDs} "Windows-x86-64")
		else()
			# default to 32-bit Windows
			set (${_outSystemIDs} "Windows")
		endif()
	elseif ("${_systemName}" STREQUAL "CYGWIN")
		if ("${_systemProcessor}" STREQUAL "x86_64")
			set (${_outSystemIDs} "Windows-x86-64")
		else()
			# default to 32-bit Windows
			set (${_outSystemIDs} "Windows")
		endif()
	elseif ("${_systemName}" STREQUAL "Darwin")
		if ("${_systemProcessor}" STREQUAL "i386")
			set (${_outSystemIDs} "MacOSX-x86")
		elseif ("${_systemProcessor}" STREQUAL "x86_64")
			set (${_outSystemIDs} "MacOSX-x86-64")
		elseif ("${_systemProcessor}" MATCHES "ppc64|powerpc64")
			set (${_outSystemIDs} "Darwin-PowerPC64")
		elseif ("${_systemProcessor}" MATCHES "ppc|powerpc")
			if (Mathematica_VERSION)
				# Mathematica versions before 6 used "Darwin" as system ID for ppc32
				if (NOT "${Mathematica_VERSION}" VERSION_LESS "6.0")
					set (${_outSystemIDs} "MacOSX")
				else()
					set (${_outSystemIDs} "Darwin")
				endif()
			else ()
				set (${_outSystemIDs} "MacOSX" "Darwin")
			endif()
		endif()
	elseif ("${_systemName}" STREQUAL "Linux")
		if ("${_systemProcessor}" MATCHES "^i.86$")
			set (${_outSystemIDs} "Linux")
		elseif ("${_systemProcessor}" MATCHES "x86_64|amd64")
			set (${_outSystemIDs} "Linux-x86-64")
		elseif ("${_systemProcessor}" STREQUAL "ia64")
			set (${_outSystemIDs} "Linux-IA64")
		elseif ("${_systemProcessor}" MATCHES "^arm")
			set (${_outSystemIDs} "Linux-ARM")
		endif()
	elseif ("${_systemName}" STREQUAL "SunOS")
		if ("${_systemProcessor}" MATCHES "^sparc")
			if (Mathematica_VERSION)
				# Mathematica versions before 6 used "UltraSPARC" as system ID for Solaris
				if (NOT "${Mathematica_VERSION}" VERSION_LESS "6.0")
					set (${_outSystemIDs} "Solaris-SPARC")
				else()
					set (${_outSystemIDs} "UltraSPARC")
				endif()
			else ()
				set (${_outSystemIDs} "Solaris-SPARC" "UltraSPARC")
			endif()
		elseif ("${_systemProcessor}" STREQUAL "x86_64")
			set (${_outSystemIDs} "Solaris-x86-64")
		endif()
	elseif ("${_systemName}" STREQUAL "AIX")
		set (${_outSystemIDs} "AIX-Power64")
	elseif ("${_systemName}" STREQUAL "HP-UX")
		set (${_outSystemIDs} "HPUX-PA64")
	elseif ("${_systemName}" STREQUAL "IRIX")
		set (${_outSystemIDs} "IRIX-MIPS64")
	endif()
endmacro(_systemNameToSystemID)

# internal macro to compute target Mathematica SystemIDs
macro (_get_system_IDs _outSystemIDs)
	if (WIN32 OR CYGWIN)
		# pointer size check is more reliable than CMAKE_SYSTEM_PROCESSOR
		if (CMAKE_SIZEOF_VOID_P EQUAL 8)
			set (${_outSystemIDs} "Windows-x86-64")
		else()
			set (${_outSystemIDs} "Windows")
		endif()
	elseif (APPLE)
		set (${_outSystemIDs} "")
		if (CMAKE_OSX_ARCHITECTURES)
			# determine System ID from specified architectures
			foreach (_arch ${CMAKE_OSX_ARCHITECTURES})
				set (_systemID "")
				_systemNameToSystemID("${CMAKE_SYSTEM_NAME}" "${_arch}" _systemID)
				if (_systemID)
					list (APPEND ${_outSystemIDs} ${_systemID})
				else()
					message (FATAL_ERROR "Unsupported Mac OS X architecture ${_arch}")
				endif()
			endforeach()
		else()
			# determine System ID by checking endianness and pointer size
			TEST_BIG_ENDIAN(_isBigEndian)
			if (_isBigEndian)
				if (CMAKE_SIZEOF_VOID_P EQUAL 8)
					set (${_outSystemIDs} "Darwin-PowerPC64")
				else()
					if (Mathematica_VERSION)
						# Mathematica versions before 6 used "Darwin" as system ID for ppc32
						if (NOT "${Mathematica_VERSION}" VERSION_LESS "6.0")
							set (${_outSystemIDs} "MacOSX")
						else()
							set (${_outSystemIDs} "Darwin")
						endif()
					else ()
						set (${_outSystemIDs} "MacOSX" "Darwin")
					endif()
				endif()
			else()
				if (CMAKE_SIZEOF_VOID_P EQUAL 8)
					set (${_outSystemIDs} "MacOSX-x86-64")
				else()
					set (${_outSystemIDs} "MacOSX-x86")
				endif()
			endif()
		endif()
	elseif (UNIX)
		if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
			# pointer size check is more reliable than CMAKE_SYSTEM_PROCESSOR
			if (CMAKE_SIZEOF_VOID_P EQUAL 8)
				set (${_outSystemIDs} "Linux-x86-64")
			else()
				set (${_outSystemIDs} "Linux")
			endif()
		else()
			_systemNameToSystemID("${CMAKE_SYSTEM_NAME}" "${CMAKE_SYSTEM_PROCESSOR}" ${_outSystemIDs})
		endif()
	else()
		set (${_outSystemIDs} "Generic")
	endif()
	list (REMOVE_DUPLICATES ${_outSystemIDs})
endmacro(_get_system_IDs)

# internal macro to compute host Mathematica SystemIDs
macro (_get_host_system_IDs _outSystemIDs)
	if (CMAKE_HOST_WIN32)
		set (${_outSystemIDs} "Windows")
		if (DEFINED ENV{PROCESSOR_ARCHITEW6432})
			if ("$ENV{PROCESSOR_ARCHITEW6432}" STREQUAL "AMD64")
				# running of WoW64, host is native 64-bit Windows
				set (${_outSystemIDs} "Windows-x86-64")
			endif()
		elseif (DEFINED ENV{PROCESSOR_ARCHITECTURE})
			if ("$ENV{PROCESSOR_ARCHITECTURE}" STREQUAL "AMD64")
				# host is native 64-bit Windows
				set (${_outSystemIDs} "Windows-x86-64")
			endif()
		endif()
	else()
		# always determine host system ID from
		# CMAKE_HOST_SYSTEM_NAME and CMAKE_HOST_SYSTEM_PROCESSOR
		if (_CMAKE_OSX_MACHINE)
			# work-around for Mac OS X, where CMAKE_HOST_SYSTEM_PROCESSOR is not always accurate
			set (_hostSystemProcessor "${_CMAKE_OSX_MACHINE}")
		else()
			set (_hostSystemProcessor "${CMAKE_HOST_SYSTEM_PROCESSOR}")
		endif()
		_systemNameToSystemID(
			"${CMAKE_HOST_SYSTEM_NAME}" "${_hostSystemProcessor}"
			_hostSystemID)
		if (NOT DEFINED _hostSystemID)
			message (FATAL_ERROR "Unsupported host platform ${CMAKE_HOST_SYSTEM_NAME}")
		endif()
		_get_compatible_system_IDs(${_hostSystemID} ${_outSystemIDs})
	endif()
endmacro()

macro (_get_supported_systemIDs _version _outSystemIDs)
	if (NOT "${_version}" VERSION_LESS "10.0")
		set (${_outSystemIDs}
			"Windows" "Windows-x86-64"
			"Linux" "Linux-x86-64" "Linux-ARM"
			"MacOSX-x86-64")
	elseif (NOT "${_version}" VERSION_LESS "9.0")
		set (${_outSystemIDs}
			"Windows" "Windows-x86-64"
			"Linux" "Linux-x86-64"
			"MacOSX-x86-64")
	elseif (NOT "${_version}" VERSION_LESS "8.0")
		set (${_outSystemIDs}
			"Windows" "Windows-x86-64"
			"Linux" "Linux-x86-64"
			"MacOSX-x86" "MacOSX-x86-64")
	elseif (NOT "${_version}" VERSION_LESS "7.0")
		set (${_outSystemIDs}
			"Windows" "Windows-x86-64"
			"Linux" "Linux-x86-64"
			"MacOSX-x86" "MacOSX-x86-64" "MacOSX"
			"Solaris-SPARC" "Solaris-x86-64")
	elseif (NOT "${_version}" VERSION_LESS "6.0")
		set (${_outSystemIDs}
			"Windows" "Windows-x86-64"
			"Linux" "Linux-x86-64" "Linux-IA64"
			"MacOSX-x86" "MacOSX-x86-64" "MacOSX"
			"Solaris-SPARC" "Solaris-x86-64"
			"AIX-Power64")
	elseif (NOT "${_version}" VERSION_LESS "5.2")
		set (${_outSystemIDs}
			"Windows" "Windows-x86-64"
			"Linux" "Linux-x86-64" "Linux-IA64"
			"MacOSX-x86" "Darwin-PowerPC64" "Darwin"
			"UltraSPARC" "Solaris-x86-64"
			"AIX-Power64" "DEC-AXP" "HPUX-PA64" "IRIX-MIPS64")
	else()
		# platforms probably supported before 5.2?
		set (${_outSystemIDs}
			"Windows"
			"Linux" "Linux-x86-64" "Linux-IA64" "Linux-PPC"
			"Darwin"
			"Solaris" "SGI"
			"IBM-RISC" "DEC-AXP" "HP-RISC" "IRIX-MIPS32" "IRIX-MIPS64")
	endif()
endmacro()

macro (_get_compatible_system_IDs _systemID _outSystemIDs)
	set (${_outSystemIDs} "")
	if ("${_systemID}" STREQUAL "Windows-x86-64")
		if (Mathematica_VERSION)
			if (NOT "${Mathematica_VERSION}" VERSION_LESS "5.2")
				# Mathematica 5.2 added support for Windows-x86-64
				list (APPEND ${_outSystemIDs} "Windows-x86-64")
			endif()
		else()
			list (APPEND ${_outSystemIDs} "Windows-x86-64")
		endif()
		# Windows x64 can run x86 through WoW64
		list (APPEND ${_outSystemIDs} "Windows")
	elseif ("${_systemID}" MATCHES "MacOSX|Darwin")
		if ("${_systemID}" MATCHES "MacOSX-x86")
			if (Mathematica_VERSION)
				# Mathematica 6 added support for MacOSX-x86-64
				if (NOT "${Mathematica_VERSION}" VERSION_LESS "6.0")
					list (APPEND ${_outSystemIDs} "MacOSX-x86-64")
				endif()
				# Mathematica 5.2 added support for MacOSX-x86
				# Mathematica 9.0 dropped support for MacOSX-x86
				if (NOT "${Mathematica_VERSION}" VERSION_LESS "5.2" AND
					"${Mathematica_VERSION}" VERSION_LESS "9.0")
					list (APPEND ${_outSystemIDs} "MacOSX-x86")
				endif()
			else()
				list (APPEND ${_outSystemIDs} "MacOSX-x86-64" "MacOSX-x86")
			endif()
		elseif ("${_systemID}" STREQUAL "Darwin-PowerPC64")
			if (Mathematica_VERSION)
				if (NOT "${Mathematica_VERSION}" VERSION_LESS "5.2" AND
					"${Mathematica_VERSION}" VERSION_LESS "6.0")
					# Only Mathematica 5.2 supports Darwin-PowerPC64
					list (APPEND ${_outSystemIDs} "Darwin-PowerPC64")
				endif()
			else()
				list (APPEND ${_outSystemIDs} "Darwin-PowerPC64")
			endif()
		endif()
		# handle ppc32 (Darwin or MacOSX)
		# Mac OS X versions before Lion support ppc32 natively or through Rosetta
		# (Mac OS X 10.7.0 is Darwin 11.0.0)
		if ("${CMAKE_HOST_SYSTEM_VERSION}" VERSION_LESS "11.0.0")
			if (Mathematica_VERSION)
				if ("${Mathematica_VERSION}" VERSION_LESS "6.0")
					# Mathematica versions before 6 used "Darwin" as system ID for ppc32
					list (APPEND ${_outSystemIDs} "Darwin")
				elseif ("${Mathematica_VERSION}" VERSION_LESS "8.0")
					# Mathematica 8 dropped support for ppc32
					list (APPEND ${_outSystemIDs} "MacOSX")
				endif()
			else()
				list (APPEND ${_outSystemIDs} "MacOSX" "Darwin")
			endif()
		endif()
	elseif ("${_systemID}" MATCHES "Linux-x86-64|Linux-IA64")
		if (Mathematica_VERSION)
			if (NOT "${Mathematica_VERSION}" VERSION_LESS "5.2")
				# Mathematica 5.2 added support for 64-bit
				list (APPEND ${_outSystemIDs} ${_systemID})
			endif()
		else()
			list (APPEND ${_outSystemIDs} ${_systemID})
		endif()
		# Linux 64-bit can run x86 through ia32-libs package
		list (APPEND ${_outSystemIDs} "Linux")
	else()
		list (APPEND ${_outSystemIDs} ${_systemID})
	endif()
	list (REMOVE_DUPLICATES ${_outSystemIDs})
endmacro()

# internal macro to compute target MathLink / WSTP DeveloperKit system ID
macro(_get_developer_kit_system_IDs _outSystemIDs)
	if (APPLE)
		if (Mathematica_VERSION)
			if ("${Mathematica_VERSION}" VERSION_LESS "9.0")
				# Mathematica versions before 9 did not have a system ID subdirectory
				set (${_outSystemIDs} "")
			else()
				# Mathematica versions after 9 have a system ID subdirectory
				set (${_outSystemIDs} "MacOSX-x86-64")
			endif()
		else()
			_get_system_IDs(${_outSystemIDs})
		endif()
	else()
		_get_system_IDs(${_outSystemIDs})
	endif()
endmacro()

# internal macro to compute host MathLink / WSTP DeveloperKit system ID
macro(_get_host_developer_kit_system_IDs _outSystemIDs)
	if (CMAKE_HOST_APPLE)
		if (Mathematica_VERSION)
			# Mathematica versions before 9 did not have a system ID subdirectory
			if ("${Mathematica_VERSION}" VERSION_LESS "9.0")
				set (${_outSystemIDs} "")
			else()
				# The MacOSX-x86-64 DeveloperKit is a universal binary with architectures i386 and x86_64
				set (${_outSystemIDs} "MacOSX-x86-64")
			endif()
		else()
			_get_host_system_IDs(${_outSystemIDs})
		endif()
	else()
		_get_host_system_IDs(${_outSystemIDs})
	endif()
endmacro()

# internal macro to compute target development flavor
macro (_get_target_flavor _outFlavor)
	if (CYGWIN)
		set (${_outFlavor} "cygwin")
	elseif (WIN32)
		if (CMAKE_SIZEOF_VOID_P EQUAL 8)
			set (${_outFlavor} "mldev64")
		else()
			set (${_outFlavor} "mldev32")
		endif()
	elseif (APPLE)
		set (${_outFlavor} "")
		if (Mathematica_VERSION)
			if (Mathematica_USE_LIBCXX_LIBRARIES AND
				NOT "${Mathematica_VERSION}" VERSION_LESS "10.0" AND
				"${Mathematica_VERSION}" VERSION_LESS "10.4")
				# Mathematica 10 added LLVM libc++ compiled version in AlternativeLibraries directory
				# Mathematica 10.4 and later only ship with LLVM libc++ compiled version
				set (${_outFlavor} "AlternativeLibraries")
			endif()
		endif()
	else()
		# no flavors on non-Windows platforms
		set (${_outFlavor} "")
	endif()
endmacro()

# internal macro to compute host development flavor
macro (_get_host_flavor _outFlavor)
	if (CYGWIN)
		set (${_outFlavor} "cygwin")
	elseif (CMAKE_HOST_WIN32)
		set (${_outFlavor} "mldev32")
		if (DEFINED ENV{PROCESSOR_ARCHITEW6432})
			if ("$ENV{PROCESSOR_ARCHITEW6432}" STREQUAL "AMD64")
				# running of WoW64, host is native 64-bit Windows
				set (${_outFlavor} "mldev64")
			endif()
		elseif (DEFINED ENV{PROCESSOR_ARCHITECTURE})
			if ("$ENV{PROCESSOR_ARCHITECTURE}" STREQUAL "AMD64")
				# host is native 64-bit Windows
				set (${_outFlavor} "mldev64")
			endif()
		endif()
	elseif (CMAKE_HOST_APPLE)
		set (${_outFlavor} "")
		if (Mathematica_VERSION)
			if (Mathematica_USE_LIBCXX_LIBRARIES AND
				NOT "${Mathematica_VERSION}" VERSION_LESS "10.0" AND
				"${Mathematica_VERSION}" VERSION_LESS "10.4")
				# Mathematica 10 added LLVM libc++ compiled version in AlternativeLibraries directory
				# Mathematica 10.4 and later only ship with LLVM libc++ compiled version
				set (${_outFlavor} "AlternativeLibraries")
			endif()
		endif()
	else()
		# no flavors on non-Windows platforms
		set (${_outFlavor} "")
	endif()
endmacro()

# internal macro to compute WolframRTL library names
macro (_get_wolfram_runtime_library_names _outLibraryNames)
	if (Mathematica_USE_STATIC_LIBRARIES)
		set (${_outLibraryNames} "WolframRTL_Static_Minimal" )
	else()
		if (Mathematica_USE_MINIMAL_LIBRARIES)
			set (${_outLibraryNames} "WolframRTL_Minimal" )
		else()
			set (${_outLibraryNames} "WolframRTL" )
		endif()
	endif()
endmacro()

# internal macro to compute MathLink library names
macro (_get_mathlink_library_names _outLibraryNames)
	if (CYGWIN)
		if (DEFINED Mathematica_MathLink_FIND_VERSION_MAJOR)
			set (${_outLibraryNames} "ML32i${Mathematica_MathLink_FIND_VERSION_MAJOR}")
		else()
			set (${_outLibraryNames} "ML32i4" "ML32i3" "ML32i2" "ML32i1")
		endif()
	elseif (WIN32)
		if (CMAKE_SIZEOF_VOID_P EQUAL 8)
			if (BORLAND)
				set (${_outLibraryNames} "ml64i3b" "ml64i2b")
			elseif (WATCOM)
				set (${_outLibraryNames} "ml64i3w" "ml64i2w")
			endif()
			# always add default Microsoft 64-bit PE libraries
			if (DEFINED Mathematica_MathLink_FIND_VERSION_MAJOR)
				if (Mathematica_USE_STATIC_LIBRARIES)
					list (APPEND ${_outLibraryNames} "ml64i${Mathematica_MathLink_FIND_VERSION_MAJOR}s")
				else()
					list (APPEND ${_outLibraryNames} "ml64i${Mathematica_MathLink_FIND_VERSION_MAJOR}m")
				endif()
			else()
				if (Mathematica_USE_STATIC_LIBRARIES)
					list (APPEND ${_outLibraryNames} "ml64i4s" "ml64i3s")
				else()
					list (APPEND ${_outLibraryNames} "ml64i4m" "ml64i3m" "ml64i2m")
				endif()
			endif()
		else()
			if (BORLAND)
				set (${_outLibraryNames} "ml32i3b" "ml32i2b" "ml32i1b")
			elseif (WATCOM)
				set (${_outLibraryNames} "ml32i3w" "ml32i2w" "ml32i1w")
			endif()
			# always add default Microsoft 32-bit PE libraries
			if (DEFINED Mathematica_MathLink_FIND_VERSION_MAJOR)
				if (Mathematica_USE_STATIC_LIBRARIES)
					list (APPEND ${_outLibraryNames} "ml32i${Mathematica_MathLink_FIND_VERSION_MAJOR}s")
				else()
					list (APPEND ${_outLibraryNames} "ml32i${Mathematica_MathLink_FIND_VERSION_MAJOR}m")
				endif()
			else()
				if (Mathematica_USE_STATIC_LIBRARIES)
					list (APPEND ${_outLibraryNames} "ml32i4s" "ml32i3s")
				else()
					list (APPEND ${_outLibraryNames} "ml32i4m" "ml32i3m" "ml32i2m" "ml32i1m")
				endif()
			endif()
		endif()
	elseif (APPLE)
		if (Mathematica_USE_STATIC_LIBRARIES)
			if (DEFINED Mathematica_MathLink_FIND_VERSION_MAJOR AND DEFINED Mathematica_MathLink_FIND_VERSION_MINOR)
				set (${_outLibraryNames} "libMLi${Mathematica_MathLink_FIND_VERSION_MAJOR}.${Mathematica_MathLink_FIND_VERSION_MINOR}.a")
			elseif (DEFINED Mathematica_MathLink_FIND_VERSION_MAJOR)
				set (${_outLibraryNames} "libMLi${Mathematica_MathLink_FIND_VERSION_MAJOR}.a")
			else()
				set (${_outLibraryNames} "libMLi4.a" "libMLi3.a" "libML.a")
			endif()
		else()
			# search for mathlink.framework
			set (${_outLibraryNames} "mathlink" "ML")
		endif()
	elseif (UNIX)
		if (Mathematica_USE_STATIC_LIBRARIES)
			set (_ext ".a")
		else()
			set (_ext ".so")
		endif()
		if (CMAKE_SIZEOF_VOID_P EQUAL 8)
			set (_arch "64")
		else()
			set (_arch "32")
		endif()
		if (DEFINED Mathematica_MathLink_FIND_VERSION_MAJOR)
			set (${_outLibraryNames} "libML${_arch}i${Mathematica_MathLink_FIND_VERSION_MAJOR}${_ext}")
		else()
			set (${_outLibraryNames} "libML${_arch}i4${_ext}" "libML${_arch}i3${_ext}" "libML${_ext}")
		endif()
	endif()
endmacro(_get_mathlink_library_names)

function (_get_mprep_output_file _templateFile _outfile)
	get_filename_component(_templateFile_name ${_templateFile} NAME)
	get_filename_component(_templateFile_ext "${_templateFile}" EXT)
	if (_templateFile_ext STREQUAL ".tmpp")
		set (${_outfile} "${_templateFile_name}.cpp" PARENT_SCOPE)
	elseif (_templateFile_ext STREQUAL ".tm++")
		set (${_outfile} "${_templateFile_name}.c++" PARENT_SCOPE)
	elseif (_templateFile_ext STREQUAL ".tmxx")
		set (${_outfile} "${_templateFile_name}.cxx" PARENT_SCOPE)
	else()
		set (${_outfile} "${_templateFile_name}.c" PARENT_SCOPE)
	endif()
endfunction()

# internal macro to compute WSTP library names
macro (_get_WSTP_library_names _outLibraryNames)
	if (CYGWIN)
		if (DEFINED Mathematica_WSTP_FIND_VERSION_MAJOR)
			set (${_outLibraryNames} "WSTP32i${Mathematica_WSTP_FIND_VERSION_MAJOR}")
		else()
			set (${_outLibraryNames} "WSTP32i4" "WSTP32i3" "WSTP32i2" "WSTP32i1")
		endif()
	elseif (WIN32)
		if (CMAKE_SIZEOF_VOID_P EQUAL 8)
			set (_arch "64")
		else()
			set (_arch "32")
		endif()
		if (DEFINED Mathematica_WSTP_FIND_VERSION_MAJOR)
			if (Mathematica_USE_STATIC_LIBRARIES)
				list (APPEND ${_outLibraryNames} "wstp${_arch}i${Mathematica_WSTP_FIND_VERSION_MAJOR}s")
			else()
				list (APPEND ${_outLibraryNames} "wstp${_arch}i${Mathematica_WSTP_FIND_VERSION_MAJOR}m")
			endif()
		else()
			if (Mathematica_USE_STATIC_LIBRARIES)
				list (APPEND ${_outLibraryNames} "wstp${_arch}i4s" "wstp${_arch}i3s")
			else()
				list (APPEND ${_outLibraryNames} "wstp${_arch}i4m" "wstp${_arch}i3m" "wstp${_arch}i2m" "wstp${_arch}i1m")
			endif()
		endif()
	elseif (APPLE)
		if (Mathematica_USE_STATIC_LIBRARIES)
			if (DEFINED Mathematica_WSTP_FIND_VERSION_MAJOR AND DEFINED Mathematica_WSTP_FIND_VERSION_MINOR)
				set (${_outLibraryNames} "libWSTPi${Mathematica_WSTP_FIND_VERSION_MAJOR}.${Mathematica_WSTP_FIND_VERSION_MINOR}.a")
			elseif (DEFINED Mathematica_WSTP_FIND_VERSION_MAJOR)
				set (${_outLibraryNames} "libWSTPi${Mathematica_WSTP_FIND_VERSION_MAJOR}.a")
			else()
				set (${_outLibraryNames} "libWSTPi4.a" "libWSTPi3.a")
			endif()
		else()
			# search for wstp.framework
			set (${_outLibraryNames} "wstp")
		endif()
	elseif (UNIX)
		if (Mathematica_USE_STATIC_LIBRARIES)
			set (_ext ".a")
		else()
			set (_ext ".so")
		endif()
		if (CMAKE_SIZEOF_VOID_P EQUAL 8)
			set (_arch "64")
		else()
			set (_arch "32")
		endif()
		if (DEFINED Mathematica_WSTP_FIND_VERSION_MAJOR)
			set (${_outLibraryNames} "libWSTP${_arch}i${Mathematica_WSTP_FIND_VERSION_MAJOR}${_ext}")
		else()
			set (${_outLibraryNames} "libWSTP${_arch}i4${_ext}" "libWSTP${_arch}i3${_ext}")
		endif()
	endif()
endmacro(_get_WSTP_library_names)

# internal macro to compute Java launcher name
macro (_get_jlink_java_name _outExecutabeName)
	if (CMAKE_HOST_WIN32)
		set (${_outExecutabeName} "java.exe")
	elseif (CMAKE_HOST_UNIX)
		set (${_outExecutabeName} "java")
	endif()
endmacro()

# internal macro to compute required WolframRTL system libraries
macro (_append_wolframlibrary_needed_system_libraries _outLibraries)
	if (UNIX)
		if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
			list (APPEND ${_outLibraries} pthread m )
		endif()
	endif()
endmacro()

# internal macro to compute required MathLink system libraries
macro (_append_mathlink_needed_system_libraries _outLibraries)
	if (APPLE)
		if (DEFINED Mathematica_MathLink_VERSION_MINOR)
			if ("${Mathematica_MathLink_VERSION_MINOR}" GREATER 18)
				# OS X MathLink API revision >= 19 has dependency on C++ standard library
				if (Mathematica_USE_LIBCXX_LIBRARIES)
					# LLVM libc++
					list (APPEND ${_outLibraries} c++ )
				else()
					# GNU libstdc++
					list (APPEND ${_outLibraries} stdc++ )
				endif()
			endif()
			if ("${Mathematica_MathLink_VERSION_MINOR}" GREATER 20)
				# Mac OS X MathLink API revision >= 21 has dependency on Core Foundation framework
				list (APPEND ${_outLibraries} "-framework Foundation" )
			endif()
		endif()
	elseif (UNIX)
		if (DEFINED Mathematica_MathLink_VERSION_MINOR)
			if ("${Mathematica_MathLink_VERSION_MINOR}" GREATER 18)
				# UNIX MathLink API revision >= 19 has dependency on GNU libstdc++
				list (APPEND ${_outLibraries} stdc++ )
			endif()
		endif()
		if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
			list (APPEND ${_outLibraries} m pthread rt )
			if (DEFINED Mathematica_MathLink_VERSION_MINOR)
				if ("${Mathematica_MathLink_VERSION_MINOR}" GREATER 24)
					# Linux MathLink API revision >= 25 has dependency on libdl and libuuid
					list (APPEND ${_outLibraries} dl uuid)
				endif()
			endif()
		elseif (CMAKE_SYSTEM_NAME STREQUAL "SunOS")
			list (APPEND ${_outLibraries} m socket nsl rt )
		elseif (CMAKE_SYSTEM_NAME STREQUAL "AIX")
			list (APPEND ${_outLibraries} m pthread )
		elseif (CMAKE_SYSTEM_NAME STREQUAL "HP-UX")
			list (APPEND ${_outLibraries}
				m /usr/lib/pa20_64/libdld.sl /usr/lib/pa20_64/libm.a pthread rt )
		elseif (CMAKE_SYSTEM_NAME STREQUAL "IRIX")
			list (APPEND ${_outLibraries} m pthread )
		endif()
	elseif (WIN32)
		if (DEFINED Mathematica_MathLink_VERSION_MINOR)
			if ("${Mathematica_MathLink_VERSION_MINOR}" GREATER 19)
				# Windows MathLink API revision >= 20 has dependency on Winsock 2
				list (APPEND ${_outLibraries} Ws2_32.lib )
			endif()
			if ("${Mathematica_MathLink_VERSION_MINOR}" GREATER 24)
				# Windows MathLink API interface >= 25 has dependency on RPC
				list (APPEND ${_outLibraries} Rpcrt4.lib )
			endif()
		endif()
	endif()
endmacro()

# internal macro to compute required WSTP system libraries
macro (_append_WSTP_needed_system_libraries _outLibraries)
	if (APPLE)
		if (DEFINED Mathematica_WSTP_VERSION_MINOR)
			if ("${Mathematica_WSTP_VERSION_MINOR}" GREATER 18)
				# OS X WSTP API revision >= 19 has dependency on C++ standard library
				if (Mathematica_USE_LIBCXX_LIBRARIES)
					# LLVM libc++
					list (APPEND ${_outLibraries} c++ )
				else()
					# GNU libstdc++
					list (APPEND ${_outLibraries} stdc++ )
				endif()
			endif()
			if ("${Mathematica_WSTP_VERSION_MINOR}" GREATER 20)
				# Mac OS X WSTP API revision >= 21 has dependency on Core Foundation framework
				list (APPEND ${_outLibraries} "-framework Foundation" )
			endif()
		endif()
	elseif (UNIX)
		if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
			if (DEFINED Mathematica_WSTP_VERSION_MINOR)
				if ("${Mathematica_WSTP_VERSION_MINOR}" GREATER 18)
					# UNIX WSTP API revision >= 19 has dependency on GNU libstdc++
					list (APPEND ${_outLibraries} stdc++ )
				endif()
			endif()
			list (APPEND ${_outLibraries} m pthread rt )
			if (DEFINED Mathematica_WSTP_VERSION_MINOR)
				if ("${Mathematica_WSTP_VERSION_MINOR}" GREATER 24)
					# Linux WSTP API revision >= 25 has dependency on libdl and libuuid
					list (APPEND ${_outLibraries} dl uuid)
				endif()
			endif()
		endif()
	elseif (WIN32)
		if (DEFINED Mathematica_WSTP_VERSION_MINOR)
			if ("${Mathematica_WSTP_VERSION_MINOR}" GREATER 19)
				# Windows WSTP API revision >= 20 has dependency on Winsock 2
				list (APPEND ${_outLibraries} Ws2_32.lib )
			endif()
			if ("${Mathematica_WSTP_VERSION_MINOR}" GREATER 24)
				# Windows WSTP API interface >= 25 has dependency on RPC
				list (APPEND ${_outLibraries} Rpcrt4.lib )
			endif()
		endif()
	endif()
endmacro()

# internal macro to return dynamic library search path environment variables on host platform
macro (_get_host_library_search_path_envvars _outVariableNames)
	set (${_outVariableNames} "")
	if (CMAKE_HOST_APPLE)
		list (APPEND ${_outVariableNames} "DYLD_FRAMEWORK_PATH" "DYLD_LIBRARY_PATH")
	elseif (CYGWIN)
		list (APPEND ${_outVariableNames} "PATH" "LD_LIBRARY_PATH")
	elseif (CMAKE_HOST_WIN32)
		list (APPEND ${_outVariableNames} "PATH")
	elseif (CMAKE_HOST_UNIX)
		if ("${CMAKE_HOST_SYSTEM_NAME}" STREQUAL "SunOS")
			list (APPEND ${_outVariableNames} "LD_LIBRARY_PATH_64")
		elseif ("${CMAKE_HOST_SYSTEM_NAME}" STREQUAL "AIX")
			list (APPEND ${_outVariableNames} "LIBPATH")
		elseif ("${CMAKE_HOST_SYSTEM_NAME}" STREQUAL "HP-UX")
			list (APPEND ${_outVariableNames} "SHLIB_PATH")
		elseif ("${CMAKE_HOST_SYSTEM_NAME}" STREQUAL "IRIX")
			list (APPEND ${_outVariableNames} "LD_LIBRARY64_PATH")
		endif()
		list (APPEND ${_outVariableNames} "LD_LIBRARY_PATH")
	endif()
endmacro()

# internal macro to convert list to a search path list for host platform
function (_to_native_path_list _outPathList)
	set (_nativePathList "")
	foreach (_path ${ARGN})
		_to_native_path("${_path}" _nativePath)
		list (APPEND _nativePathList "${_nativePath}")
	endforeach()
	if (CMAKE_HOST_UNIX)
		string (REPLACE ";" ":" _nativePathList "${_nativePathList}")
	elseif (CMAKE_HOST_WIN32)
		# prevent CMake from interpreting ; as a list separator
		string (REPLACE ";" "\\;" _nativePathList "${_nativePathList}")
	endif()
	set (${_outPathList} "${_nativePathList}" PARENT_SCOPE)
endfunction()

function (_to_cmake_path_list _outPathList)
	set (_cmakePathList "")
	foreach (_path ${ARGN})
		_to_cmake_path("${_path}" _cmakePath)
		list (APPEND _cmakePathList "${_cmakePath}")
	endforeach()
	if (CMAKE_HOST_UNIX)
		string (REPLACE ";" ":" _cmakePathList "${_cmakePathList}")
	elseif (CMAKE_HOST_WIN32)
		# prevent CMake from interpreting ; as a list separator
		string (REPLACE ";" "\\;" _cmakePathList "${_cmakePathList}")
	endif()
	set (${_outPathList} "${_cmakePathList}" PARENT_SCOPE)
endfunction()

# internal macro to select runtime libraries according to build type
macro (_select_configuration_run_time_dirs _outRuntimeDirs)
	set (${_outRuntimeDirs} ${Mathematica_RUNTIME_LIBRARY_DIRS})
	if (DEFINED CMAKE_BUILD_TYPE)
		if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
			set (${_outRuntimeDirs} ${Mathematica_RUNTIME_LIBRARY_DIRS_DEBUG})
		endif()
	endif()
endmacro()

# internal macro to set up Mathematica host system IDs
macro (_setup_mathematica_systemIDs)
	_get_system_IDs(Mathematica_SYSTEM_IDS)
	# default target platform system ID is first one in Mathematica_SYSTEM_IDS
	list(GET Mathematica_SYSTEM_IDS 0 Mathematica_SYSTEM_ID)
	if (COMMAND Mathematica_EXECUTE)
		# determine true host system ID which depends on both Mathematica version
		# and OS variant by running Mathematica kernel
		Mathematica_EXECUTE(
			CODE "Print[StandardForm[$SystemID]]"
			OUTPUT_VARIABLE Mathematica_KERNEL_HOST_SYSTEM_ID
			CACHE DOC "Actual Mathematica host system ID."
			TIMEOUT 10)
		if (NOT Mathematica_KERNEL_HOST_SYSTEM_ID)
			message (WARNING "Cannot accurately determine Mathematica host system ID.")
		endif()
	endif()
	if (Mathematica_KERNEL_HOST_SYSTEM_ID)
		if (Mathematica_KERNEL_HOST_SYSTEM_ID MATCHES "[a-zA-Z0-9_-]+")
			set (Mathematica_HOST_SYSTEM_ID "${Mathematica_KERNEL_HOST_SYSTEM_ID}")
		else()
			unset (Mathematica_KERNEL_HOST_SYSTEM_ID CACHE)
		endif()
	else()
		# guess host system ID from the environment
		_get_host_system_IDs(_HostSystemIDs)
		# default to first ID in _HostSystemIDs
		list (GET _HostSystemIDs 0 Mathematica_HOST_SYSTEM_ID)
	endif()
	_get_compatible_system_IDs(${Mathematica_HOST_SYSTEM_ID} Mathematica_HOST_SYSTEM_IDS)
endmacro()

# internal macro to set up Mathematica creation ID
macro (_setup_mathematica_creationID)
	if (DEFINED Mathematica_ROOT_DIR)
		if (EXISTS "${Mathematica_ROOT_DIR}/.CreationID")
			# parse hidden CreationID file
			file (STRINGS "${Mathematica_ROOT_DIR}/.CreationID" Mathematica_CREATION_ID REGEX "[0-9]+")
		elseif (CMAKE_HOST_APPLE AND EXISTS "${Mathematica_ROOT_DIR}/Contents/Info.plist")
			execute_process(
				COMMAND "grep" "--after-context=1" "CFBundleShortVersionString"
					"${Mathematica_ROOT_DIR}/Contents/Info.plist"
				TIMEOUT 10 OUTPUT_VARIABLE _versionStr ERROR_QUIET)
			if (_versionStr MATCHES "\\.([0-9]+)</string>")
				# OS X Info.plist CFBundleShortVersionString has Creation ID as last version component
				set (Mathematica_CREATION_ID "${CMAKE_MATCH_1}")
			else()
				set (_versionLine "")
			endif()
		endif()
	endif()
	if (NOT DEFINED Mathematica_CREATION_ID AND DEFINED Mathematica_CREATION_ID_LAST)
		set (Mathematica_CREATION_ID ${Mathematica_CREATION_ID_LAST})
	endif()
endmacro()

# internal macro to set up Mathematica base directory variable
macro (_setup_mathematica_base_directory)
	if (COMMAND Mathematica_EXECUTE)
		# determine true $BaseDirectory
		Mathematica_EXECUTE(
			CODE "Print[StandardForm[$BaseDirectory]]"
			OUTPUT_VARIABLE Mathematica_KERNEL_BASE_DIR
			CACHE DOC "Actual Mathematica $BaseDirectory."
			TIMEOUT 10)
		if (NOT Mathematica_KERNEL_BASE_DIR)
			message (WARNING "Cannot accurately determine Mathematica $BaseDirectory.")
		endif()
	endif()
	if (Mathematica_KERNEL_BASE_DIR)
		if (IS_ABSOLUTE "${Mathematica_KERNEL_BASE_DIR}")
			set (Mathematica_BASE_DIR "${Mathematica_KERNEL_BASE_DIR}")
		else()
			unset (Mathematica_KERNEL_BASE_DIR CACHE)
		endif()
	else ()
		# guess Mathematica_BASE_DIR from environment
		# environment variable MATHEMATICA_BASE may override default
		# $BaseDirectory, see http://reference.wolfram.com/language/tutorial/ConfigurationFiles.html
		if (DEFINED ENV{MATHEMATICA_BASE})
			set (Mathematica_BASE_DIR "$ENV{MATHEMATICA_BASE}")
		elseif (CMAKE_HOST_WIN32 OR CYGWIN)
			if (DEFINED $ENV{PROGRAMDATA})
				set (Mathematica_BASE_DIR "$ENV{PROGRAMDATA}\\Mathematica")
			elseif (DEFINED ENV{ALLUSERSAPPDATA})
				set (Mathematica_BASE_DIR "$ENV{ALLUSERSAPPDATA}\\Mathematica")
			elseif (DEFINED ENV{USERPROFILE} AND
					DEFINED ENV{ALLUSERSPROFILE} AND
					DEFINED ENV{APPDATA})
				string (REPLACE "$ENV{USERPROFILE}" "$ENV{ALLUSERSPROFILE}"
					Mathematica_BASE_DIR "$ENV{APPDATA}\\Mathematica")
			endif()
		elseif (CMAKE_HOST_APPLE)
			set (Mathematica_BASE_DIR "/Library/Mathematica")
		elseif (CMAKE_HOST_UNIX)
			set (Mathematica_BASE_DIR "/usr/share/Mathematica")
		endif()
	endif()
	if (Mathematica_BASE_DIR)
		get_filename_component(Mathematica_BASE_DIR "${Mathematica_BASE_DIR}" ABSOLUTE)
		_to_cmake_path("${Mathematica_BASE_DIR}" Mathematica_BASE_DIR)
	else()
		set (Mathematica_BASE_DIR "Mathematica_BASE_DIR-NOTFOUND")
		message (WARNING "Cannot determine Mathematica base directory.")
	endif()
endmacro()

# internal macro to set up Mathematica user base directory variable
macro (_setup_mathematica_userbase_directory)
	if (COMMAND Mathematica_EXECUTE)
		# determine true $UserBaseDirectory
		Mathematica_EXECUTE(
			CODE "Print[StandardForm[$UserBaseDirectory]]"
			OUTPUT_VARIABLE Mathematica_KERNEL_USERBASE_DIR
			CACHE DOC "Actual Mathematica $UserBaseDirectory."
			TIMEOUT 10)
		if (NOT Mathematica_KERNEL_USERBASE_DIR)
			message (WARNING "Cannot accurately determine Mathematica $UserBaseDirectory.")
		endif()
	endif()
	if (Mathematica_KERNEL_USERBASE_DIR)
		if (IS_ABSOLUTE "${Mathematica_KERNEL_USERBASE_DIR}")
			set (Mathematica_USERBASE_DIR "${Mathematica_KERNEL_USERBASE_DIR}")
		else()
			unset (Mathematica_KERNEL_USERBASE_DIR CACHE)
		endif()
	else ()
		# guess Mathematica_USERBASE_DIR from environment
		# environment variable MATHEMATICA_USERBASE may override default
		# $UserBaseDirectory, see http://reference.wolfram.com/language/tutorial/ConfigurationFiles.html
		if (DEFINED ENV{MATHEMATICA_USERBASE})
			set (Mathematica_USERBASE_DIR "$ENV{MATHEMATICA_USERBASE}")
		elseif (CMAKE_HOST_WIN32 OR CYGWIN)
			if (DEFINED ENV{APPDATA})
				set (Mathematica_USERBASE_DIR "$ENV{APPDATA}\\Mathematica")
			endif()
		elseif (CMAKE_HOST_APPLE)
			if (DEFINED ENV{HOME})
				set (Mathematica_USERBASE_DIR "$ENV{HOME}/Library/Mathematica")
			endif()
		elseif (CMAKE_HOST_UNIX)
			if (DEFINED ENV{HOME})
				set (Mathematica_USERBASE_DIR "$ENV{HOME}/.Mathematica")
			endif()
		endif()
	endif()
	if (Mathematica_USERBASE_DIR)
		get_filename_component(Mathematica_USERBASE_DIR "${Mathematica_USERBASE_DIR}" ABSOLUTE)
		_to_cmake_path("${Mathematica_USERBASE_DIR}" Mathematica_USERBASE_DIR)
	else()
		set (Mathematica_USERBASE_DIR "Mathematica_USERBASE_DIR-NOTFOUND")
		message (WARNING "Cannot determine Mathematica user base directory.")
	endif()
endmacro()

# internal macro to setup FindMathematica option variables
macro (_setup_findmathematica_options)
	if (NOT DEFINED Mathematica_USE_STATIC_LIBRARIES_INIT)
		if (DEFINED Mathematica_USE_STATIC_LIBRARIES)
			set (Mathematica_USE_STATIC_LIBRARIES_INIT ${Mathematica_USE_STATIC_LIBRARIES})
		else()
			set (Mathematica_USE_STATIC_LIBRARIES_INIT FALSE)
		endif()
	endif()
	option (Mathematica_USE_STATIC_LIBRARIES
		"prefer static Mathematica libraries to dynamic libraries?"
		${Mathematica_USE_STATIC_LIBRARIES_INIT})
	if (NOT DEFINED Mathematica_USE_MINIMAL_LIBRARIES_INIT)
		if (DEFINED Mathematica_USE_MINIMAL_LIBRARIES)
			set (Mathematica_USE_MINIMAL_LIBRARIES_INIT ${Mathematica_USE_MINIMAL_LIBRARIES})
		else()
			set (Mathematica_USE_MINIMAL_LIBRARIES_INIT FALSE)
		endif()
	endif()
	option (Mathematica_USE_MINIMAL_LIBRARIES
		"prefer minimal Mathematica libraries to full libraries?"
		${Mathematica_USE_MINIMAL_LIBRARIES_INIT})
	if (NOT DEFINED Mathematica_USE_LIBCXX_LIBRARIES_INIT)
		if (DEFINED Mathematica_USE_LIBCXX_LIBRARIES)
			set (Mathematica_USE_LIBCXX_LIBRARIES_INIT ${Mathematica_USE_LIBCXX_LIBRARIES})
		else()
			# starting with OS X 10.9, Clang uses libc++ by default
			if (APPLE AND NOT "${CMAKE_SYSTEM_VERSION}" VERSION_LESS "13.0.0" AND "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
				set (Mathematica_USE_LIBCXX_LIBRARIES_INIT TRUE)
			else()
				set (Mathematica_USE_LIBCXX_LIBRARIES_INIT FALSE)
			endif()
		endif()
	endif()
	option (Mathematica_USE_LIBCXX_LIBRARIES
		"prefer Mathematica libraries linked with LLVM libc++ to those linked with GNU libstdc++?"
		${Mathematica_USE_LIBCXX_LIBRARIES_INIT})
	if (NOT DEFINED Mathematica_DEBUG_INIT)
		if (DEFINED Mathematica_DEBUG)
			set (Mathematica_DEBUG_INIT ${Mathematica_DEBUG})
		else()
			set (Mathematica_DEBUG_INIT FALSE)
		endif()
	endif()
	option (Mathematica_DEBUG
		"enable FindMathematica debugging output?"
		${Mathematica_DEBUG_INIT})
endmacro()

# internal macro to find Mathematica installation
macro (_find_mathematica)
	_get_host_frontend_names(_FrontEndExecutables)
	_get_host_kernel_names(_KernelExecutables)
	if (Mathematica_DEBUG)
		message (STATUS "FrontEndExecutables ${_FrontEndExecutables}")
		message (STATUS "KernelExecutables ${_KernelExecutables}")
	endif()
	set (_helpStr "Mathematica host installation root directory.")
	if (NOT DEFINED Mathematica_HOST_ROOT_DIR)
		set (_doSearch TRUE)
	elseif (NOT EXISTS "${Mathematica_HOST_ROOT_DIR}")
		set (_doSearch TRUE)
	else()
		set (_doSearch FALSE)
	endif()
	if (_doSearch)
		_get_search_paths(_SearchPaths)
		_get_program_names(_ProgramNames)
		if (Mathematica_DEBUG)
			message (STATUS "SearchPaths ${_SearchPaths}")
			message (STATUS "ProgramNames ${_ProgramNames}")
			message (STATUS "KernelExecutables ${_KernelExecutables}")
		endif()
		find_path (Mathematica_HOST_ROOT_DIR
			NAMES ${_KernelExecutables}
			PATH_SUFFIXES ${_ProgramNames}
			PATHS ${_SearchPaths} ENV MATHEMATICA_HOME
			DOC "${_helpStr}"
			NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH
		)
	else()
		# preserve pre-defined value, but set correct type and help string
		set_property(CACHE Mathematica_HOST_ROOT_DIR PROPERTY TYPE PATH)
		set_property(CACHE Mathematica_HOST_ROOT_DIR PROPERTY HELPSTRING "${_helpStr}")
	endif()
	# Mathematica_ROOT_DIR is initialized to Mathematica_HOST_ROOT_DIR by default
	# upon cross-compiling Mathematica_ROOT_DIR needs to be manually set to the correct
	# Mathematica installation folder for the target platform
	set (_helpStr "Mathematica target installation root directory.")
	if (NOT DEFINED Mathematica_ROOT_DIR)
		set (Mathematica_ROOT_DIR ${Mathematica_HOST_ROOT_DIR} CACHE PATH "${_helpStr}")
	elseif (NOT EXISTS "${Mathematica_ROOT_DIR}")
		set (Mathematica_ROOT_DIR ${Mathematica_HOST_ROOT_DIR} CACHE PATH "${_helpStr}")
	else()
		# preserve pre-defined value, but set correct type and help string
		set_property(CACHE Mathematica_ROOT_DIR PROPERTY TYPE PATH)
		set_property(CACHE Mathematica_ROOT_DIR PROPERTY HELPSTRING "${_helpStr}")
	endif()
	find_program (Mathematica_KERNEL_EXECUTABLE
		NAMES ${_KernelExecutables}
		HINTS ${Mathematica_HOST_ROOT_DIR}
		DOC "Mathematica kernel executable."
		NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH
	)
	find_program (Mathematica_FRONTEND_EXECUTABLE
		NAMES ${_FrontEndExecutables}
		HINTS ${Mathematica_HOST_ROOT_DIR}
		DOC "Mathematica front end executable."
		NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH
	)
	find_path (Mathematica_INCLUDE_DIR
		NAMES "mdefs.h"
		HINTS
			"${Mathematica_ROOT_DIR}/SystemFiles/IncludeFiles"
			"${Mathematica_ROOT_DIR}/Contents/SystemFiles/IncludeFiles"
		PATH_SUFFIXES "C"
		DOC "Mathematica C language definitions include directory."
		NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH
	)
	if (Mathematica_INCLUDE_DIR)
		set (Mathematica_INCLUDE_DIRS ${Mathematica_INCLUDE_DIR})
	else()
		set (Mathematica_INCLUDE_DIRS "")
	endif()
	set (Mathematica_LIBRARIES "")
	set (Mathematica_LIBRARY_DIRS "")
	set (Mathematica_RUNTIME_LIBRARY_DIRS "")
	set (Mathematica_RUNTIME_LIBRARY_DIRS_DEBUG "")
endmacro(_find_mathematica)

# internal macro to init _LIBRARIES variable from given _LIBRARY variable
macro (_setup_libraries_var _library_var _libraries_var)
	if (APPLE)
		# handle universal builds under Mac OS X
		# we need to add a library for each architecture
		_get_system_IDs(_SystemIDs)
		foreach (_systemID IN LISTS _SystemIDs)
			if ("${${_library_var}}" MATCHES "/${_systemID}/")
				set (_primarySystemID "${_systemID}")
			endif()
		endforeach()
		if (_primarySystemID)
			set (${_libraries_var} "")
			foreach (_systemID IN LISTS _SystemIDs)
				string (REPLACE "/${_primarySystemID}/" "/${_systemID}/" _library
					"${${_library_var}}")
				if (EXISTS "${_library}")
					list (APPEND ${_libraries_var} "${_library}")
				endif()
			endforeach()
		else()
			set (${_libraries_var} ${${_library_var}})
		endif()
	else()
		set (${_libraries_var} ${${_library_var}})
	endif()
endmacro()

# internal macro to find Wolfram Library inside Mathematica installation
macro (_find_wolframlibrary)
	if (NOT DEFINED Mathematica_ROOT_DIR)
		_find_mathematica()
	endif()
	_get_system_IDs(_SystemIDs)
	_get_wolfram_runtime_library_names(_WolframRuntimeLibraryNames)
	if (Mathematica_DEBUG)
		message (STATUS "WolframLibrary SystemID ${_SystemIDs}")
		message (STATUS "WolframRuntimeLibraryNames ${_WolframRuntimeLibraryNames}")
	endif()
	set (_findLibraryPrefixesSave "${CMAKE_FIND_LIBRARY_PREFIXES}")
	set (_findLibrarySuffixesSave "${CMAKE_FIND_LIBRARY_SUFFIXES}")
	if (CYGWIN)
		# Wolfram RTL library names do not follow UNIX conventions under Cygwin
		set (CMAKE_FIND_LIBRARY_PREFIXES "")
		set (CMAKE_FIND_LIBRARY_SUFFIXES ".lib")
	endif()
	find_library (Mathematica_WolframLibrary_LIBRARY
		NAMES ${_WolframRuntimeLibraryNames}
		HINTS
			"${Mathematica_ROOT_DIR}/SystemFiles/Libraries"
			"${Mathematica_ROOT_DIR}/Contents/SystemFiles/Libraries"
		PATH_SUFFIXES ${_SystemIDs}
		DOC "Mathematica Wolfram Runtime Library."
		NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH
	)
	find_path (Mathematica_WolframLibrary_INCLUDE_DIR
		NAMES "WolframLibrary.h" "WolframRTL.h"
		HINTS
			"${Mathematica_ROOT_DIR}/SystemFiles/IncludeFiles"
			"${Mathematica_ROOT_DIR}/Contents/SystemFiles/IncludeFiles"
		PATH_SUFFIXES "C"
		DOC "Mathematica WolframLibrary include directory."
		NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH
	)
	if (Mathematica_WolframLibrary_INCLUDE_DIR)
		list (APPEND Mathematica_INCLUDE_DIRS ${Mathematica_WolframLibrary_INCLUDE_DIR})
	endif()
	set (CMAKE_FIND_LIBRARY_PREFIXES "${_findLibraryPrefixesSave}")
	set (CMAKE_FIND_LIBRARY_SUFFIXES "${_findLibrarySuffixesSave}")
endmacro()

# internal macro to find MathLink SDK inside Mathematica installation
macro (_find_mathlink)
	_get_developer_kit_system_IDs(_SystemIDs)
	_get_host_developer_kit_system_IDs(_HostSystemIDs)
	_get_target_flavor(_MathLinkFlavor)
	_get_host_flavor(_HostMathLinkFlavor)
	_get_mathlink_library_names(_MathLinkLibraryNames)
	if (NOT DEFINED Mathematica_ROOT_DIR OR
		NOT DEFINED Mathematica_HOST_ROOT_DIR)
		_find_mathematica()
	endif()
	if (Mathematica_DEBUG)
		message (STATUS "MathLink Target DeveloperKit SystemID ${_SystemIDs} ${_MathLinkFlavor}")
		message (STATUS "MathLink Host DeveloperKit SystemID ${_HostSystemIDs} ${_HostMathLinkFlavor}")
		message (STATUS "MathLink Library Names ${_MathLinkLibraryNames}")
	endif()
	find_path (Mathematica_MathLink_ROOT_DIR
		NAMES "CompilerAdditions"
		HINTS
			"${Mathematica_ROOT_DIR}/SystemFiles/Links/MathLink/DeveloperKit"
			"${Mathematica_ROOT_DIR}/Contents/SystemFiles/Links/MathLink/DeveloperKit"
			"${Mathematica_ROOT_DIR}/AddOns/MathLink/DeveloperKit"
		PATH_SUFFIXES ${_SystemIDs}
		DOC "MathLink target SDK root directory."
		NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH
	)
	if (_MathLinkFlavor)
		set (_CompilerAdditions
			"${Mathematica_MathLink_ROOT_DIR}/CompilerAdditions/${_MathLinkFlavor}"
			"${Mathematica_MathLink_ROOT_DIR}/CompilerAdditions")
	else()
		set (_CompilerAdditions "${Mathematica_MathLink_ROOT_DIR}/CompilerAdditions")
	endif()
	find_path (Mathematica_MathLink_HOST_ROOT_DIR
		NAMES "CompilerAdditions"
		HINTS
			"${Mathematica_HOST_ROOT_DIR}/SystemFiles/Links/MathLink/DeveloperKit"
			"${Mathematica_HOST_ROOT_DIR}/Contents/SystemFiles/Links/MathLink/DeveloperKit"
			"${Mathematica_HOST_ROOT_DIR}/AddOns/MathLink/DeveloperKit"
		PATH_SUFFIXES ${_HostSystemIDs}
		DOC "MathLink host SDK root directory."
		NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH
	)
	if (_HostMathLinkFlavor)
		set (_HostCompilerAdditions
			"${Mathematica_MathLink_HOST_ROOT_DIR}/CompilerAdditions/${_HostMathLinkFlavor}"
			"${Mathematica_MathLink_HOST_ROOT_DIR}/CompilerAdditions")
	else()
		set (_HostCompilerAdditions "${Mathematica_MathLink_HOST_ROOT_DIR}/CompilerAdditions")
	endif()
	if (Mathematica_DEBUG)
		message (STATUS "MathLink CompilerAdditions ${_CompilerAdditions}")
		message (STATUS "MathLink HostCompilerAdditions ${_HostCompilerAdditions}")
	endif()
	if (APPLE)
		set (_findFrameWorkSave "${CMAKE_FIND_FRAMEWORK}")
		if (Mathematica_USE_STATIC_LIBRARIES)
			set (CMAKE_FIND_FRAMEWORK "LAST")
		else()
			set (CMAKE_FIND_FRAMEWORK "FIRST")
		endif()
	endif()
	find_program (Mathematica_MathLink_MPREP_EXECUTABLE
		NAMES "mprep"
		HINTS ${_HostCompilerAdditions}
		PATH_SUFFIXES "bin"
		DOC "MathLink template file preprocessor executable."
		NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH
	)
	find_library (Mathematica_MathLink_LIBRARY
		NAMES ${_MathLinkLibraryNames}
		HINTS ${_CompilerAdditions}
		PATH_SUFFIXES "lib"
		DOC "MathLink library to link against."
		NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH
	)
	find_path (Mathematica_MathLink_INCLUDE_DIR
		NAMES "mathlink.h"
		HINTS ${_CompilerAdditions}
		PATH_SUFFIXES "include"
		DOC "Path to the MathLink include directory."
		NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH
	)
	if (APPLE AND DEFINED Mathematica_MathLink_FIND_VERSION_MAJOR AND IS_DIRECTORY "${Mathematica_MathLink_LIBRARY}")
		if (DEFINED Mathematica_MathLink_FIND_VERSION_MINOR)
			set (_frameworkVersionSubDir "${Mathematica_MathLink_LIBRARY}/Versions/${Mathematica_MathLink_FIND_VERSION_MAJOR}.${Mathematica_MathLink_FIND_VERSION_MINOR}")
		else()
			set (_frameworkVersionSubDir "${Mathematica_MathLink_LIBRARY}/Versions/${Mathematica_MathLink_FIND_VERSION_MAJOR}.[0-9]+")
		endif()
		file (GLOB _versionedLibrary "${_frameworkVersionSubDir}/mathlink")
		if (_versionedLibrary)
			# use last if there are multiple
			list (GET _versionedLibrary -1 _versionedLibrary)
			set (Mathematica_MathLink_LIBRARY "${_versionedLibrary}" CACHE FILEPATH "MathLink library to link against." FORCE)
		endif()
		file (GLOB _versionedHeaderDir "${_frameworkVersionSubDir}/Headers")
		if (_versionedHeaderDir)
			set (Mathematica_MathLink_INCLUDE_DIR "${_versionedHeaderDir}" CACHE FILEPATH "Path to the MathLink include directory." FORCE)
		endif()
	endif()
	find_path (Mathematica_MathLink_HOST_INCLUDE_DIR
		NAMES "mathlink.h"
		HINTS ${_HostCompilerAdditions}
		PATH_SUFFIXES "include"
		DOC "Path to the MathLink host include directory."
		NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH
	)
	if (APPLE)
		set (CMAKE_FIND_FRAMEWORK "${_findFrameWorkSave}")
	endif()
	if (Mathematica_MathLink_INCLUDE_DIR)
		list (APPEND Mathematica_INCLUDE_DIRS ${Mathematica_MathLink_INCLUDE_DIR})
	endif()
endmacro(_find_mathlink)

# internal macro to find WSTP SDK inside Mathematica installation
macro (_find_WSTP)
	_get_developer_kit_system_IDs(_SystemIDs)
	_get_host_developer_kit_system_IDs(_HostSystemIDs)
	_get_target_flavor(_WSTPFlavor)
	_get_host_flavor(_HostWSTPFlavor)
	_get_WSTP_library_names(_WSTPLibraryNames)
	if (NOT DEFINED Mathematica_ROOT_DIR OR
		NOT DEFINED Mathematica_HOST_ROOT_DIR)
		_find_mathematica()
	endif()
	if (Mathematica_DEBUG)
		message (STATUS "WSTP Target DeveloperKit SystemID ${_SystemIDs} ${_WSTPFlavor}")
		message (STATUS "WSTP Host DeveloperKit SystemID ${_HostSystemIDs} ${_HostWSTPFlavor}")
		message (STATUS "WSTP Library Names ${_WSTPLibraryNames}")
	endif()
	find_path (Mathematica_WSTP_ROOT_DIR
		NAMES "CompilerAdditions"
		HINTS
			"${Mathematica_ROOT_DIR}/SystemFiles/Links/WSTP/DeveloperKit"
			"${Mathematica_ROOT_DIR}/Contents/SystemFiles/Links/WSTP/DeveloperKit"
		PATH_SUFFIXES ${_SystemIDs}
		DOC "WSTP target SDK root directory."
		NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH
	)
	if (_WSTPFlavor)
		set (_CompilerAdditions
			"${Mathematica_WSTP_ROOT_DIR}/CompilerAdditions/${_WSTPFlavor}"
			"${Mathematica_WSTP_ROOT_DIR}/CompilerAdditions")
	else()
		set (_CompilerAdditions "${Mathematica_WSTP_ROOT_DIR}/CompilerAdditions")
	endif()
	find_path (Mathematica_WSTP_HOST_ROOT_DIR
		NAMES "CompilerAdditions"
		HINTS
			"${Mathematica_HOST_ROOT_DIR}/SystemFiles/Links/WSTP/DeveloperKit"
			"${Mathematica_HOST_ROOT_DIR}/Contents/SystemFiles/Links/WSTP/DeveloperKit"
		PATH_SUFFIXES ${_HostSystemIDs}
		DOC "WSTP host SDK root directory."
		NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH
	)
	if (_HostWSTPFlavor)
		set (_HostCompilerAdditions
			"${Mathematica_WSTP_HOST_ROOT_DIR}/CompilerAdditions/${_HostWSTPFlavor}"
			"${Mathematica_WSTP_HOST_ROOT_DIR}/CompilerAdditions")
	else()
		set (_HostCompilerAdditions "${Mathematica_WSTP_HOST_ROOT_DIR}/CompilerAdditions")
	endif()
	if (Mathematica_DEBUG)
		message (STATUS "WSTP CompilerAdditions ${_CompilerAdditions}")
		message (STATUS "WSTP HostCompilerAdditions ${_HostCompilerAdditions}")
	endif()
	if (APPLE)
		set (_findFrameWorkSave "${CMAKE_FIND_FRAMEWORK}")
		if (Mathematica_USE_STATIC_LIBRARIES)
			set (CMAKE_FIND_FRAMEWORK "LAST")
		else()
			set (CMAKE_FIND_FRAMEWORK "FIRST")
		endif()
	endif()
	find_program (Mathematica_WSTP_WSPREP_EXECUTABLE
		NAMES "wsprep"
		HINTS ${_HostCompilerAdditions}
		PATH_SUFFIXES "bin"
		DOC "WSTP template file preprocessor executable."
		NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH
	)
	find_library (Mathematica_WSTP_LIBRARY
		NAMES ${_WSTPLibraryNames}
		HINTS ${_CompilerAdditions}
		PATH_SUFFIXES "lib"
		DOC "WSTP library to link against."
		NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH
	)
	find_path (Mathematica_WSTP_INCLUDE_DIR
		NAMES "wstp.h"
		HINTS ${_CompilerAdditions}
		PATH_SUFFIXES "include"
		DOC "Path to the WSTP include directory."
		NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH
	)
	if (APPLE AND DEFINED Mathematica_WSTP_FIND_VERSION_MAJOR AND IS_DIRECTORY "${Mathematica_WSTP_LIBRARY}")
		if (DEFINED Mathematica_WSTP_FIND_VERSION_MINOR)
			set (_frameworkVersionSubDir "${Mathematica_WSTP_LIBRARY}/Versions/${Mathematica_WSTP_FIND_VERSION_MAJOR}.${Mathematica_WSTP_FIND_VERSION_MINOR}")
		else()
			set (_frameworkVersionSubDir "${Mathematica_WSTP_LIBRARY}/Versions/${Mathematica_WSTP_FIND_VERSION_MAJOR}.[0-9]+")
		endif()
		file (GLOB _versionedLibrary "${_frameworkVersionSubDir}/wstp")
		if (_versionedLibrary)
			# use last if there are multiple
			list (GET _versionedLibrary -1 _versionedLibrary)
			set (Mathematica_WSTP_LIBRARY "${_versionedLibrary}" CACHE FILEPATH "WSTP library to link against." FORCE)
		endif()
		file (GLOB _versionedHeaderDir "${_frameworkVersionSubDir}/Headers")
		if (_versionedHeaderDir)
			set (Mathematica_WSTP_INCLUDE_DIR "${_versionedHeaderDir}" CACHE FILEPATH "Path to the WSTP include directory." FORCE)
		endif()
	endif()
	find_path (Mathematica_WSTP_HOST_INCLUDE_DIR
		NAMES "wstp.h"
		HINTS ${_HostCompilerAdditions}
		PATH_SUFFIXES "include"
		DOC "Path to the WSTP host include directory."
		NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH
	)
	if (APPLE)
		set (CMAKE_FIND_FRAMEWORK "${_findFrameWorkSave}")
	endif()
	if (Mathematica_WSTP_INCLUDE_DIR)
		list (APPEND Mathematica_INCLUDE_DIRS ${Mathematica_WSTP_INCLUDE_DIR})
	endif()
endmacro(_find_WSTP)

# internal macro to find J/Link SDK inside Mathematica installation
macro (_find_jlink)
	if (NOT DEFINED Mathematica_ROOT_DIR)
		_find_mathematica()
	endif()
	_get_system_IDs(_SystemIDs)
	_get_host_system_IDs(_HostSystemIDs)
	_get_jlink_java_name(_JLinkJavaNames)
	if (Mathematica_DEBUG)
		message (STATUS "J/Link Target SystemID ${_SystemIDs}")
		message (STATUS "J/Link Host SystemID ${_HostSystemIDs}")
		message (STATUS "JLinkJavaName ${_JLinkJavaNames}")
	endif()
	find_path (Mathematica_JLink_PACKAGE_DIR
		NAMES "JLink.jar"
		HINTS
			"${Mathematica_ROOT_DIR}/SystemFiles/Links/JLink"
			"${Mathematica_ROOT_DIR}/Contents/SystemFiles/Links/JLink"
			"${Mathematica_ROOT_DIR}/AddOns/JLink"
		DOC "J/Link SDK root directory."
		NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH
	)
	if (EXISTS "${Mathematica_JLink_PACKAGE_DIR}")
		set (Mathematica_JLink_JAR_FILE "${Mathematica_JLink_PACKAGE_DIR}/JLink.jar")
	else()
		set (Mathematica_JLink_JAR_FILE "Mathematica_JLink_JAR_FILE-NOTFOUND")
	endif()
	set (_findLibraryPrefixesSave "${CMAKE_FIND_LIBRARY_PREFIXES}")
	set (_findLibrarySuffixesSave "${CMAKE_FIND_LIBRARY_SUFFIXES}")
	if (APPLE)
		set (CMAKE_FIND_LIBRARY_PREFIXES "lib")
		set (CMAKE_FIND_LIBRARY_SUFFIXES ".jnilib")
	elseif (WIN32)
		set (CMAKE_FIND_LIBRARY_PREFIXES "")
		set (CMAKE_FIND_LIBRARY_SUFFIXES ".dll")
	endif()
	find_library (Mathematica_JLink_RUNTIME_LIBRARY
		NAMES "JLinkNativeLibrary"
		HINTS "${Mathematica_JLink_PACKAGE_DIR}/SystemFiles/Libraries"
		PATHS ENV JLINK_LIB_DIR
		PATH_SUFFIXES ${_SystemIDs}
		DOC "J/Link native library."
		NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH
	)
	set (CMAKE_FIND_LIBRARY_PREFIXES "${_findLibraryPrefixesSave}")
	set (CMAKE_FIND_LIBRARY_SUFFIXES "${_findLibrarySuffixesSave}")
	if (CMAKE_HOST_APPLE)
		if (EXISTS "${Mathematica_HOST_ROOT_DIR}/Contents/SystemFiles/Java")
			set (_mmaJavaHome "${Mathematica_HOST_ROOT_DIR}/Contents/SystemFiles/Java")
		else()
			# OS X versions of Mathematica earlier than 10 did not have a JVM bundled
			# but used the Java JVM pre-installed on system
			set (_mmaJavaHome "${Mathematica_HOST_ROOT_DIR}/SystemFiles/Java")
			if (DEFINED Mathematica_VERSION)
				if ("${Mathematica_VERSION}" VERSION_LESS "10.0")
					# use java_home to find path to JVM installed on system
					find_program(Mathematica_JAVA_HOME_EXECUTABLE "java_home" PATHS "/usr/libexec/")
					mark_as_advanced(Mathematica_JAVA_HOME_EXECUTABLE)
					if (Mathematica_JAVA_HOME_EXECUTABLE)
						execute_process(
							COMMAND "${Mathematica_JAVA_HOME_EXECUTABLE}" "--version" "1.6"
							TIMEOUT 10 OUTPUT_VARIABLE _mmaJavaHome ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
					endif()
				endif()
			endif()
		endif()
	else()
		set (_mmaJavaHome "${Mathematica_HOST_ROOT_DIR}/SystemFiles/Java")
	endif()
	find_program (Mathematica_JLink_JAVA_EXECUTABLE
		NAMES "bin/${_JLinkJavaNames}"
		HINTS "${_mmaJavaHome}"
		PATH_SUFFIXES ${_HostSystemIDs}
		DOC "J/Link Java launcher."
		NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH
	)
endmacro()

# internal macro to find MUnit package
macro (_find_munit_package)
	if (COMMAND Mathematica_FIND_PACKAGE)
		Mathematica_FIND_PACKAGE(Mathematica_MUnit_PACKAGE_FILE "MUnit`MUnit`")
		# determine enclosing MUnit package directory
		if (Mathematica_MUnit_PACKAGE_FILE)
			Mathematica_GET_PACKAGE_DIR(Mathematica_MUnit_PACKAGE_DIR "${Mathematica_MUnit_PACKAGE_FILE}")
		endif()
	endif()
	if (NOT DEFINED Mathematica_MUnit_PACKAGE_DIR)
		set (Mathematica_MUnit_PACKAGE_DIR "Mathematica_MUnit_PACKAGE_DIR-NOTFOUND")
	endif()
endmacro()

# internal macro to find LibaryLink package
macro (_find_librarylink_package)
	if (COMMAND Mathematica_FIND_PACKAGE)
		Mathematica_FIND_PACKAGE(Mathematica_LibraryLink_PACKAGE_FILE "LibraryLink`LibraryLink`")
		# determine enclosing LibraryLink package directory
		if (Mathematica_LibraryLink_PACKAGE_FILE)
			Mathematica_GET_PACKAGE_DIR(Mathematica_LibraryLink_PACKAGE_DIR "${Mathematica_LibraryLink_PACKAGE_FILE}")
		endif()
	endif()
	if (NOT DEFINED Mathematica_LibraryLink_PACKAGE_DIR)
		set (Mathematica_LibraryLink_PACKAGE_DIR "Mathematica_LibraryLink_PACKAGE_DIR-NOTFOUND")
	endif()
endmacro()

# internal helper macro to setup version related variables from existing _VERSION variable
macro (_setup_package_version_variables _packageName)
	if (DEFINED ${_packageName}_VERSION)
		string (REGEX MATCHALL "[0-9]+" _versionComponents "${${_packageName}_VERSION}")
		list (LENGTH _versionComponents _len)
		if (_len GREATER 0)
			list(GET _versionComponents 0 ${_packageName}_VERSION_MAJOR)
		endif()
		if (_len GREATER 1)
			list(GET _versionComponents 1 ${_packageName}_VERSION_MINOR)
		endif()
		if (_len GREATER 2)
			list(GET _versionComponents 2 ${_packageName}_VERSION_PATCH)
		endif()
		if (_len GREATER 3)
			list(GET _versionComponents 3 ${_packageName}_VERSION_TWEAK)
		endif()
		set (${_packageName}_VERSION_COUNT ${_len})
	else()
		set (${_packageName}_VERSION_COUNT 0)
		set (${_packageName}_VERSION "")
	endif()
	if (NOT DEFINED ${_packageName}_VERSION_STRING)
		set (${_packageName}_VERSION_STRING ${${_packageName}_VERSION})
	endif()
endmacro()

# internal macro to setup Mathematica version related variables
macro (_setup_mathematica_version_variables)
	if (NOT Mathematica_VERSION)
		if (Mathematica_ROOT_DIR AND
			EXISTS "${Mathematica_ROOT_DIR}/.VersionID")
			# parse version number from hidden VersionID and PatchLevel files
			file (STRINGS "${Mathematica_ROOT_DIR}/.VersionID" _versionLine)
			if (EXISTS "${Mathematica_ROOT_DIR}/.PatchLevel")
				file (STRINGS "${Mathematica_ROOT_DIR}/.PatchLevel" _patchLevel)
				if (_versionLine MATCHES ".+" AND _patchLevel MATCHES ".+")
					set (_versionLine "${_versionLine}.${_patchLevel}")
				endif()
			endif()
		elseif (CMAKE_HOST_APPLE AND Mathematica_ROOT_DIR AND
			EXISTS "${Mathematica_ROOT_DIR}/Contents/Info.plist")
			execute_process(
				COMMAND "grep" "--after-context=1" "CFBundleShortVersionString"
					"${Mathematica_ROOT_DIR}/Contents/Info.plist"
				TIMEOUT 10 OUTPUT_VARIABLE _versionStr ERROR_QUIET)
			if (_versionStr MATCHES "<string>([0-9]+\\.[0-9]+\\.[0-9]+)")
				set (_versionLine "${CMAKE_MATCH_1}")
			else()
				set (_versionLine "")
			endif()
		elseif (DEFINED Mathematica_MathLink_INCLUDE_DIR AND
			EXISTS "${Mathematica_MathLink_INCLUDE_DIR}/mathlink.h")
			# parse version number from mathlink.h
			file (STRINGS "${Mathematica_MathLink_INCLUDE_DIR}/mathlink.h" _versionLine
				REGEX ".*define.*MLMATHVERSION.*")
		elseif (DEFINED Mathematica_MathLink_HOST_INCLUDE_DIR AND
			EXISTS "${Mathematica_MathLink_HOST_INCLUDE_DIR}/mathlink.h")
			# parse version number from mathlink.h
			file (STRINGS "${Mathematica_MathLink_HOST_INCLUDE_DIR}/mathlink.h" _versionLine
				REGEX ".*define.*MLMATHVERSION.*")
		else()
			set (_versionLine "")
		endif()
		if (_versionLine MATCHES ".+")
			string (REGEX REPLACE "[^0-9]*([0-9]+(\\.[0-9]+)*).*" "\\1" _versionStr "${_versionLine}")
			if (DEFINED _versionStr)
				set (Mathematica_VERSION "${_versionStr}"
					CACHE INTERNAL "Mathematica version." FORCE)
			endif()
		endif()
	endif()
	_setup_package_version_variables(Mathematica)
endmacro()

# internal macro to setup WolframLibrary version related variables
macro (_setup_wolframlibrary_version_variables)
	if (NOT Mathematica_WolframLibrary_VERSION)
		set (_file "${Mathematica_WolframLibrary_INCLUDE_DIR}/WolframLibrary.h")
		if (EXISTS "${_file}")
			file (STRINGS "${_file}" _versionLine REGEX ".*define.*WolframLibraryVersion.*")
			if (_versionLine)
				string (REGEX REPLACE "[^0-9]*([0-9]+(\\.[0-9]+)*).*" "\\1" _versionStr "${_versionLine}")
				if (DEFINED _versionStr)
					set (Mathematica_WolframLibrary_VERSION "${_versionStr}"
						CACHE INTERNAL "WolframLibrary version." FORCE)
				endif()
			endif()
		endif()
	endif()
	_setup_package_version_variables(Mathematica_WolframLibrary)
endmacro()

# internal macro to setup MathLink version related variables
macro (_setup_mathlink_version_variables)
	if (NOT Mathematica_MathLink_VERSION)
		set (_file "${Mathematica_MathLink_INCLUDE_DIR}/mathlink.h")
		if (EXISTS "${_file}")
			if (DEFINED Mathematica_MathLink_FIND_VERSION_MAJOR)
				set (_mlInterface "${Mathematica_MathLink_FIND_VERSION_MAJOR}")
			else()
				file (STRINGS "${_file}" _mlInterfaceLine REGEX ".*define.*MLINTERFACE.*")
				string (REGEX REPLACE "[^0-9]*([0-9]+).*" "\\1" _mlInterface
					${_mlInterfaceLine})
			endif()
			file (STRINGS "${_file}" _mlRevisionLine REGEX ".*define.*MLREVISION.*")
			string (REGEX REPLACE "[^0-9]*([0-9]+).*" "\\1" _mlRevision
				${_mlRevisionLine})
			if (DEFINED _mlInterface AND DEFINED _mlRevision)
				set (_versionStr "${_mlInterface}.${_mlRevision}")
				set (Mathematica_MathLink_VERSION "${_versionStr}"
					CACHE INTERNAL "MathLink version." FORCE)
			endif()
		endif()
	endif()
	_setup_package_version_variables(Mathematica_MathLink)
endmacro()

# internal macro to setup WSTP version related variables
macro (_setup_WSTP_version_variables)
	if (NOT Mathematica_WSTP_VERSION)
		set (_file "${Mathematica_WSTP_INCLUDE_DIR}/wstp.h")
		if (EXISTS "${_file}")
			if (DEFINED Mathematica_WSTP_FIND_VERSION_MAJOR)
				set (_wstpInterface "${Mathematica_WSTP_FIND_VERSION_MAJOR}")
			else()
				file (STRINGS "${_file}" _wstpInterfaceLine REGEX ".*define.*(WS|ML)INTERFACE.*")
				if (_wstpInterfaceLine)
					string (REGEX REPLACE "[^0-9]*([0-9]+).*" "\\1" _wstpInterface
						${_wstpInterfaceLine})
				endif()
			endif()
			file (STRINGS "${_file}" _wstpRevisionLine REGEX ".*define.*(WS|ML)REVISION.*")
			string (REGEX REPLACE "[^0-9]*([0-9]+).*" "\\1" _wstpRevision
				${_wstpRevisionLine})
			if (DEFINED _wstpInterface AND DEFINED _wstpRevision)
				set (_versionStr "${_wstpInterface}.${_wstpRevision}")
				set (Mathematica_WSTP_VERSION "${_versionStr}"
					CACHE INTERNAL "WSTP version." FORCE)
			endif()
		endif()
	endif()
	_setup_package_version_variables(Mathematica_WSTP)
endmacro()

# internal macro to setup J/Link version related variables
macro (_setup_jlink_version_variables)
	if (NOT Mathematica_JLink_VERSION)
		set (_file "${Mathematica_JLink_PACKAGE_DIR}/Source/Java/com/wolfram/jlink/KernelLink.java")
		if (EXISTS "${_file}")
			file (STRINGS "${_file}" _versionLine REGEX ".*String.*VERSION.*")
			string (REGEX REPLACE "[^0-9]*([0-9]+(\\.[0-9]+)*).*" "\\1" _versionStr "${_versionLine}")
			if (DEFINED _versionStr)
				set (Mathematica_JLink_VERSION "${_versionStr}"
					CACHE INTERNAL "J/Link version." FORCE)
			endif()
		endif()
	endif()
	_setup_package_version_variables(Mathematica_JLink)
endmacro()

# internal macro to setup MUnit version related variables
macro (_setup_munit_package_version_variables)
	if (NOT Mathematica_MUnit_VERSION)
		set (_file "${Mathematica_MUnit_PACKAGE_FILE}")
		if (EXISTS "${_file}")
			file (STRINGS "${_file}" _mUnitVersionNumberLine REGEX ".*`\\$VersionNumber.*")
			file (STRINGS "${_file}" _mUnitReleaseNumberLine REGEX ".*`\\$ReleaseNumber.*")
			file (STRINGS "${_file}" _mUnitVersionLine REGEX ".*`\\$Version.*")
			string (REGEX REPLACE "[^0-9]*([0-9]+\\.[0-9]+).*" "\\1" _mUnitVersionNumber
				${_mUnitVersionNumberLine})
			string (REGEX REPLACE "[^0-9]*([0-9]+).*" "\\1" _mUnitReleaseNumber
				${_mUnitReleaseNumberLine})
			if (DEFINED _mUnitVersionNumber AND DEFINED _mUnitReleaseNumber)
				set (_versionStr "${_mUnitVersionNumber}.${_mUnitReleaseNumber}")
				set (Mathematica_MUnit_VERSION "${_versionStr}"
					CACHE INTERNAL "MUnit version." FORCE)
			endif()
		endif()
	endif()
	_setup_package_version_variables(Mathematica_MUnit)
endmacro()

# internal macro to setup WolframLibrary library related variables
macro (_setup_wolframlibrary_library_variables)
	if (Mathematica_WolframLibrary_LIBRARY)
		set (Mathematica_WolframLibrary_RUNTIME_LIBRARY_DIRS "")
		set (Mathematica_WolframLibrary_RUNTIME_LIBRARY_DIRS_DEBUG "")
		_setup_libraries_var(Mathematica_WolframLibrary_LIBRARY Mathematica_WolframLibrary_LIBRARIES)
		foreach (_library ${Mathematica_WolframLibrary_LIBRARIES})
			get_filename_component (_libraryDir ${_library} DIRECTORY)
			list (APPEND Mathematica_LIBRARY_DIRS ${_libraryDir})
			if (NOT Mathematica_USE_STATIC_LIBRARIES)
				list (APPEND Mathematica_WolframLibrary_RUNTIME_LIBRARY_DIRS ${_libraryDir})
				list (APPEND Mathematica_WolframLibrary_RUNTIME_LIBRARY_DIRS_DEBUG ${_libraryDir})
				list (APPEND Mathematica_RUNTIME_LIBRARY_DIRS ${_libraryDir})
				list (APPEND Mathematica_RUNTIME_LIBRARY_DIRS_DEBUG ${_libraryDir})
			endif()
		endforeach()
		if (NOT APPLE)
			# kernel binaries dir on Windows and Linux contains additional runtime libraries (e.g., Intel MKL)
			foreach (_systemID ${Mathematica_SYSTEM_IDS})
				set (_kernelBinariesDir "${Mathematica_ROOT_DIR}/SystemFiles/Kernel/Binaries/${_systemID}")
				if (EXISTS "${_kernelBinariesDir}")
					list (APPEND Mathematica_LIBRARY_DIRS ${_kernelBinariesDir})
					if (NOT Mathematica_USE_STATIC_LIBRARIES)
						list (APPEND Mathematica_WolframLibrary_RUNTIME_LIBRARY_DIRS "${_kernelBinariesDir}")
						list (APPEND Mathematica_WolframLibrary_RUNTIME_LIBRARY_DIRS_DEBUG "${_kernelBinariesDir}")
						list (APPEND Mathematica_RUNTIME_LIBRARY_DIRS "${_kernelBinariesDir}")
						list (APPEND Mathematica_RUNTIME_LIBRARY_DIRS_DEBUG "${_kernelBinariesDir}")
					endif()
				endif()
			endforeach()
		endif()
		_append_wolframlibrary_needed_system_libraries(Mathematica_WolframLibrary_LIBRARIES)
		list (REMOVE_DUPLICATES Mathematica_WolframLibrary_RUNTIME_LIBRARY_DIRS)
		list (REMOVE_DUPLICATES Mathematica_WolframLibrary_RUNTIME_LIBRARY_DIRS_DEBUG)
		list (APPEND Mathematica_LIBRARIES ${Mathematica_WolframLibrary_LIBRARIES})
	endif()
endmacro()

# internal macro to setup MathLink library related variables
macro (_setup_mathlink_library_variables)
	if (Mathematica_MathLink_LIBRARY)
		_setup_libraries_var(Mathematica_MathLink_LIBRARY Mathematica_MathLink_LIBRARIES)
		if (DEFINED Mathematica_MathLink_VERSION_MAJOR)
			set (Mathematica_MathLink_DEFINITIONS "-DMLINTERFACE=${Mathematica_MathLink_VERSION_MAJOR}")
		elseif (DEFINED Mathematica_MathLink_FIND_VERSION_MAJOR)
			set (Mathematica_MathLink_DEFINITIONS "-DMLINTERFACE=${Mathematica_MathLink_FIND_VERSION_MAJOR}")
		else()
			set (Mathematica_MathLink_DEFINITIONS "")
		endif()
		set (Mathematica_MathLink_RUNTIME_LIBRARY_DIRS "")
		set (Mathematica_MathLink_RUNTIME_LIBRARY_DIRS_DEBUG "")
		if (APPLE)
			set (Mathematica_MathLink_LINKER_FLAGS "")
			foreach (_library ${Mathematica_MathLink_LIBRARIES})
				get_filename_component (_libraryDir ${_library} DIRECTORY)
				list (APPEND Mathematica_LIBRARY_DIRS ${_libraryDir})
			endforeach()
			# for OS X we have to add the MathLink CompilerAdditions directory which contains the MathLink framework
			_get_target_flavor(_MathLinkFlavor)
			if (_MathLinkFlavor)
				set (_CompilerAdditions "${Mathematica_MathLink_ROOT_DIR}/CompilerAdditions/${_MathLinkFlavor}")
			else()
				set (_CompilerAdditions "${Mathematica_MathLink_ROOT_DIR}/CompilerAdditions")
			endif()
			if (IS_DIRECTORY "${_CompilerAdditions}")
				list (APPEND Mathematica_MathLink_RUNTIME_LIBRARY_DIRS "${_CompilerAdditions}")
				list (APPEND Mathematica_MathLink_RUNTIME_LIBRARY_DIRS_DEBUG "${_CompilerAdditions}")
				list (APPEND Mathematica_RUNTIME_LIBRARY_DIRS "${_CompilerAdditions}")
				list (APPEND Mathematica_RUNTIME_LIBRARY_DIRS_DEBUG "${_CompilerAdditions}")
			endif()
		elseif (UNIX)
			set (Mathematica_MathLink_LINKER_FLAGS "")
			foreach (_library ${Mathematica_MathLink_LIBRARIES})
				get_filename_component (_libraryDir ${_library} DIRECTORY)
				list (APPEND Mathematica_LIBRARY_DIRS ${_libraryDir})
				if (NOT Mathematica_USE_STATIC_LIBRARIES)
					list (APPEND Mathematica_MathLink_RUNTIME_LIBRARY_DIRS ${_libraryDir})
					list (APPEND Mathematica_MathLink_RUNTIME_LIBRARY_DIRS_DEBUG ${_libraryDir})
					list (APPEND Mathematica_RUNTIME_LIBRARY_DIRS ${_libraryDir})
					list (APPEND Mathematica_RUNTIME_LIBRARY_DIRS_DEBUG ${_libraryDir})
				endif()
			endforeach()
		elseif (WIN32)
			set (Mathematica_MathLink_LINKER_FLAGS "")
			foreach (_library ${Mathematica_MathLink_LIBRARIES})
				get_filename_component (_libraryDir ${_library} DIRECTORY)
				list (APPEND Mathematica_LIBRARY_DIRS ${_libraryDir})
			endforeach()
			# Windows MathLink SDK has runtime DLLs in a separate directory
			set (_runtimeDir "${Mathematica_MathLink_ROOT_DIR}/SystemAdditions")
			if (IS_DIRECTORY "${_runtimeDir}")
				list (APPEND Mathematica_MathLink_RUNTIME_LIBRARY_DIRS "${_runtimeDir}")
				list (APPEND Mathematica_RUNTIME_LIBRARY_DIRS "${_runtimeDir}")
			endif()
			# Windows MathLink SDK also ships with debug DLLs in AlternativeComponents
			set (_runtimeDir "${Mathematica_MathLink_ROOT_DIR}/AlternativeComponents/DebugLibraries")
			if (IS_DIRECTORY "${_runtimeDir}")
				list (APPEND Mathematica_MathLink_RUNTIME_LIBRARY_DIRS_DEBUG "${_runtimeDir}")
				list (APPEND Mathematica_RUNTIME_LIBRARY_DIRS_DEBUG "${_runtimeDir}")
			endif()
		endif()
		_append_mathlink_needed_system_libraries(Mathematica_MathLink_LIBRARIES)
		list (REMOVE_DUPLICATES Mathematica_MathLink_RUNTIME_LIBRARY_DIRS)
		list (REMOVE_DUPLICATES Mathematica_MathLink_RUNTIME_LIBRARY_DIRS_DEBUG)
		list (APPEND Mathematica_LIBRARIES ${Mathematica_MathLink_LIBRARIES})
	endif()
endmacro()

# internal macro to setup WSTP library related variables
macro (_setup_WSTP_library_variables)
	if (Mathematica_WSTP_LIBRARY)
		_setup_libraries_var(Mathematica_WSTP_LIBRARY Mathematica_WSTP_LIBRARIES)
		if (DEFINED Mathematica_WSTP_VERSION_MAJOR)
			set (Mathematica_WSTP_DEFINITIONS "-DWSINTERFACE=${Mathematica_WSTP_VERSION_MAJOR}")
		elseif (DEFINED Mathematica_WSTP_FIND_VERSION_MAJOR)
			set (Mathematica_WSTP_DEFINITIONS "-DWSINTERFACE=${Mathematica_WSTP_FIND_VERSION_MAJOR}")
		else()
			set (Mathematica_WSTP_DEFINITIONS "")
		endif()
		set (Mathematica_WSTP_RUNTIME_LIBRARY_DIRS "")
		set (Mathematica_WSTP_RUNTIME_LIBRARY_DIRS_DEBUG "")
		if (APPLE)
			set (Mathematica_WSTP_LINKER_FLAGS "")
			foreach (_library ${Mathematica_WSTP_LIBRARIES})
				get_filename_component (_libraryDir ${_library} DIRECTORY)
				list (APPEND Mathematica_LIBRARY_DIRS ${_libraryDir})
			endforeach()
			# for OS X we have to add the WSTP CompilerAdditions directory which contains the WSTP framework
			_get_target_flavor(_WSTPFlavor)
			if (_WSTPFlavor)
				set (_CompilerAdditions "${Mathematica_WSTP_ROOT_DIR}/CompilerAdditions/${_WSTPFlavor}")
			else()
				set (_CompilerAdditions "${Mathematica_WSTP_ROOT_DIR}/CompilerAdditions")
			endif()
			if (IS_DIRECTORY "${_CompilerAdditions}")
				list (APPEND Mathematica_WSTP_RUNTIME_LIBRARY_DIRS ${_libraryDir})
				list (APPEND Mathematica_WSTP_RUNTIME_LIBRARY_DIRS_DEBUG ${_libraryDir})
				list (APPEND Mathematica_RUNTIME_LIBRARY_DIRS "${_CompilerAdditions}")
				list (APPEND Mathematica_RUNTIME_LIBRARY_DIRS_DEBUG "${_CompilerAdditions}")
			endif()
		elseif (UNIX)
			set (Mathematica_WSTP_LINKER_FLAGS "")
			foreach (_library ${Mathematica_WSTP_LIBRARIES})
				get_filename_component (_libraryDir ${_library} DIRECTORY)
				list (APPEND Mathematica_LIBRARY_DIRS ${_libraryDir})
				if (NOT Mathematica_USE_STATIC_LIBRARIES)
					list (APPEND Mathematica_WSTP_RUNTIME_LIBRARY_DIRS ${_libraryDir})
					list (APPEND MathematicaWSTP_RUNTIME_LIBRARY_DIRS_DEBUG ${_libraryDir})
					list (APPEND Mathematica_RUNTIME_LIBRARY_DIRS ${_libraryDir})
					list (APPEND Mathematica_RUNTIME_LIBRARY_DIRS_DEBUG ${_libraryDir})
				endif()
			endforeach()
		elseif (WIN32)
			set (Mathematica_WSTP_LINKER_FLAGS "")
			foreach (_library ${Mathematica_WSTP_LIBRARIES})
				get_filename_component (_libraryDir ${_library} DIRECTORY)
				list (APPEND Mathematica_LIBRARY_DIRS ${_libraryDir})
			endforeach()
			# Windows WSTP SDK has runtime DLLs in a separate directory
			set (_runtimeDir "${Mathematica_WSTP_ROOT_DIR}/SystemAdditions")
			if (IS_DIRECTORY "${_runtimeDir}")
				list (APPEND Mathematica_WSTP_RUNTIME_LIBRARY_DIRS "${_runtimeDir}")
				list (APPEND Mathematica_RUNTIME_LIBRARY_DIRS "${_runtimeDir}")
			endif()
			# Windows WSTP SDK also ships with debug DLLs in AlternativeComponents
			set (_runtimeDir "${Mathematica_WSTP_ROOT_DIR}/AlternativeComponents/DebugLibraries")
			if (IS_DIRECTORY "${_runtimeDir}")
				list (APPEND Mathematica_WSTP_RUNTIME_LIBRARY_DIRS_DEBUG "${_runtimeDir}")
				list (APPEND Mathematica_RUNTIME_LIBRARY_DIRS_DEBUG "${_runtimeDir}")
			endif()
		endif()
		_append_WSTP_needed_system_libraries(Mathematica_WSTP_LIBRARIES)
		list (REMOVE_DUPLICATES Mathematica_WSTP_RUNTIME_LIBRARY_DIRS)
		list (REMOVE_DUPLICATES Mathematica_WSTP_RUNTIME_LIBRARY_DIRS_DEBUG)
		list (APPEND Mathematica_LIBRARIES ${Mathematica_WSTP_LIBRARIES})
	endif()
endmacro()

# internal macro to log used variables
macro (_log_used_variables)
	if (Mathematica_DEBUG)
		message (STATUS "Executing on ${CMAKE_HOST_SYSTEM}, ${CMAKE_HOST_SYSTEM_NAME}, ${CMAKE_HOST_SYSTEM_PROCESSOR}, ${CMAKE_HOST_SYSTEM_VERSION}")
		message (STATUS "Compiling for ${CMAKE_SYSTEM}, ${CMAKE_SYSTEM_NAME}, ${CMAKE_SYSTEM_PROCESSOR}, ${CMAKE_SYSTEM_VERSION}")
		message (STATUS "Configuration: ${CMAKE_BUILD_TYPE}, ${CMAKE_CONFIGURATION_TYPES}")
		message (STATUS "Configuration directory: ${CMAKE_CFG_INTDIR}")
		message (STATUS "Project source dir: ${PROJECT_SOURCE_DIR}")
		message (STATUS "Project binary dir: ${PROJECT_BINARY_DIR}")
		message (STATUS "Cross compiling: ${CMAKE_CROSSCOMPILING}")
		message (STATUS "Library prefixes: ${CMAKE_FIND_LIBRARY_PREFIXES}")
		message (STATUS "Library suffixes: ${CMAKE_FIND_LIBRARY_SUFFIXES}")
		message (STATUS "Current file: ${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}")
		message (STATUS "Parent file: ${CMAKE_PARENT_LIST_FILE}")
		message (STATUS "Find version: ${Mathematica_FIND_VERSION}")
		message (STATUS "Find exact: ${Mathematica_FIND_VERSION_EXACT}")
		message (STATUS "Find quietly: ${Mathematica_FIND_QUIETLY}")
		message (STATUS "Find required: ${Mathematica_FIND_REQUIRED}")
		message (STATUS "Find components: ${Mathematica_FIND_COMPONENTS}")
		message (STATUS "Find required MathLink: ${Mathematica_FIND_REQUIRED_MathLink}")
		message (STATUS "Find MathLink interface version: ${Mathematica_MathLink_FIND_VERSION_MAJOR}")
		message (STATUS "Find MathLink revision number: ${Mathematica_MathLink_FIND_VERSION_MINOR}")
		message (STATUS "Find required WSTP: ${Mathematica_FIND_REQUIRED_WSTP}")
		message (STATUS "Find WSTP interface version: ${Mathematica_WSTP_FIND_VERSION_MAJOR}")
		message (STATUS "Find WSTP revision number: ${Mathematica_WSTP_FIND_VERSION_MINOR}")
		message (STATUS "Find required WolframLibrary: ${Mathematica_FIND_REQUIRED_WolframLibrary}")
		message (STATUS "Find required J/Link: ${Mathematica_FIND_REQUIRED_JLink}")
		message (STATUS "Find required MUnit: ${Mathematica_FIND_REQUIRED_MUnit}")
		message (STATUS "Use static libraries: ${Mathematica_USE_STATIC_LIBRARIES}")
		message (STATUS "Use minimal libraries: ${Mathematica_USE_MINIMAL_LIBRARIES}")
	endif()
endmacro()

# internal macro to log found variables
macro (_log_found_variables)
	if (Mathematica_DEBUG)
		message (STATUS "Mathematica CMake module dir ${Mathematica_CMAKE_MODULE_DIR}")
		if (Mathematica_FOUND)
			message (STATUS "Mathematica ${Mathematica_VERSION} found")
			message (STATUS "Mathematica creation ID ${Mathematica_CREATION_ID}")
			message (STATUS "Mathematica target root dir ${Mathematica_ROOT_DIR}")
			message (STATUS "Mathematica host root dir ${Mathematica_HOST_ROOT_DIR}")
			message (STATUS "Mathematica host MathLink include dir ${Mathematica_MathLink_HOST_INCLUDE_DIR}")
			message (STATUS "Mathematica host WSTP include dir ${Mathematica_WSTP_HOST_INCLUDE_DIR}")
			message (STATUS "Mathematica kernel executable ${Mathematica_KERNEL_EXECUTABLE}")
			message (STATUS "Mathematica frontend executable ${Mathematica_FRONTEND_EXECUTABLE}")
			message (STATUS "Mathematica target system ID ${Mathematica_SYSTEM_ID}")
			message (STATUS "Mathematica target system IDs ${Mathematica_SYSTEM_IDS}")
			message (STATUS "Mathematica host system ID ${Mathematica_HOST_SYSTEM_ID}")
			message (STATUS "Mathematica host system IDs ${Mathematica_HOST_SYSTEM_IDS}")
			message (STATUS "Mathematica base directory ${Mathematica_BASE_DIR}")
			message (STATUS "Mathematica user base directory ${Mathematica_USERBASE_DIR}")
			message (STATUS "Mathematica include dir ${Mathematica_INCLUDE_DIR}")
			message (STATUS "Mathematica include dirs ${Mathematica_INCLUDE_DIRS}")
			message (STATUS "Mathematica libraries ${Mathematica_LIBRARIES}")
			message (STATUS "Mathematica library dirs ${Mathematica_LIBRARY_DIRS}")
			message (STATUS "Mathematica runtime library dirs ${Mathematica_RUNTIME_LIBRARY_DIRS}")
			message (STATUS "Mathematica runtime debug library dirs ${Mathematica_RUNTIME_LIBRARY_DIRS_DEBUG}")
		else()
			message (STATUS "Mathematica not found")
		endif()
		if (Mathematica_WolframLibrary_FOUND)
			message (STATUS "WolframLibrary ${Mathematica_WolframLibrary_VERSION} found")
			message (STATUS "WolframLibrary include dir ${Mathematica_WolframLibrary_INCLUDE_DIR}")
			message (STATUS "WolframLibrary library ${Mathematica_WolframLibrary_LIBRARY}")
			message (STATUS "WolframLibrary libraries ${Mathematica_WolframLibrary_LIBRARIES}")
			message (STATUS "WolframLibrary runtime library dirs ${Mathematica_WolframLibrary_RUNTIME_LIBRARY_DIRS}")
			message (STATUS "WolframLibrary runtime debug library dirs ${Mathematica_WolframLibrary_RUNTIME_LIBRARY_DIRS_DEBUG}")
			message (STATUS "LibraryLink package dir ${Mathematica_LibraryLink_PACKAGE_DIR}")
		else()
			message (STATUS "WolframLibrary not found")
		endif()
		if (Mathematica_MathLink_FOUND)
			message (STATUS "MathLink ${Mathematica_MathLink_VERSION} found")
			message (STATUS "MathLink target root dir ${Mathematica_MathLink_ROOT_DIR}")
			message (STATUS "MathLink host root dir ${Mathematica_MathLink_HOST_ROOT_DIR}")
			message (STATUS "MathLink include dir ${Mathematica_MathLink_INCLUDE_DIR}")
			message (STATUS "MathLink library ${Mathematica_MathLink_LIBRARY}")
			message (STATUS "MathLink libraries ${Mathematica_MathLink_LIBRARIES}")
			message (STATUS "MathLink mprep executable ${Mathematica_MathLink_MPREP_EXECUTABLE}")
			message (STATUS "MathLink definitions ${Mathematica_MathLink_DEFINITIONS}")
			message (STATUS "MathLink linker flags ${Mathematica_MathLink_LINKER_FLAGS}")
			message (STATUS "MathLink runtime library dirs ${Mathematica_MathLink_RUNTIME_LIBRARY_DIRS}")
			message (STATUS "MathLink runtime debug library dirs ${Mathematica_MathLink_RUNTIME_LIBRARY_DIRS_DEBUG}")
		else()
			message (STATUS "MathLink not found")
		endif()
		if (Mathematica_WSTP_FOUND)
			message (STATUS "WSTP ${Mathematica_WSTP_VERSION} found")
			message (STATUS "WSTP target root dir ${Mathematica_WSTP_ROOT_DIR}")
			message (STATUS "WSTP host root dir ${Mathematica_WSTP_HOST_ROOT_DIR}")
			message (STATUS "WSTP include dir ${Mathematica_WSTP_INCLUDE_DIR}")
			message (STATUS "WSTP library ${Mathematica_WSTP_LIBRARY}")
			message (STATUS "WSTP libraries ${Mathematica_WSTP_LIBRARIES}")
			message (STATUS "WSTP wsprep executable ${Mathematica_WSTP_WSPREP_EXECUTABLE}")
			message (STATUS "WSTP definitions ${Mathematica_WSTP_DEFINITIONS}")
			message (STATUS "WSTP linker flags ${Mathematica_WSTP_LINKER_FLAGS}")
			message (STATUS "WSTP runtime library dirs ${Mathematica_WSTP_RUNTIME_LIBRARY_DIRS}")
			message (STATUS "WSTP runtime debug library dirs ${Mathematica_WSTP_RUNTIME_LIBRARY_DIRS_DEBUG}")
		else()
			message (STATUS "WSTP not found")
		endif()
		if (Mathematica_JLink_FOUND)
			message (STATUS "J/Link ${Mathematica_JLink_VERSION} found")
			message (STATUS "J/Link package dir ${Mathematica_JLink_PACKAGE_DIR}")
			message (STATUS "J/Link JAR file ${Mathematica_JLink_JAR_FILE}")
			message (STATUS "J/Link native library ${Mathematica_JLink_RUNTIME_LIBRARY}")
			message (STATUS "J/Link java launcher ${Mathematica_JLink_JAVA_EXECUTABLE}")
		else()
			message (STATUS "J/Link not found")
		endif()
		if (Mathematica_MUnit_FOUND)
			message (STATUS "MUnit ${Mathematica_MUnit_VERSION} found")
			message (STATUS "MUnit package dir ${Mathematica_MUnit_PACKAGE_DIR}")
		else()
			message (STATUS "MUnit not found")
		endif()
	endif()
	if (DEFINED Mathematica_VERSION_MAJOR AND
		DEFINED Mathematica_VERSION_MINOR AND
		DEFINED Mathematica_SYSTEM_IDS)
		if (APPLE AND "${Mathematica_VERSION_MAJOR}" EQUAL 5 AND "${Mathematica_VERSION_MINOR}" EQUAL 2)
			foreach (_systemID ${Mathematica_SYSTEM_IDS})
				if ("${_systemID}" STREQUAL "MacOSX-x86-64")
					message (WARNING "Mathematica 5.2 for Mac OS X does not support x86_64, run cmake with option -DCMAKE_OSX_ARCHITECTURES=i386.")
				endif()
			endforeach()
		endif()
	endif()
	if (CYGWIN AND CMAKE_COMPILER_IS_GNUCC AND Mathematica_WolframLibrary_FOUND)
		if ("${CMAKE_C_COMPILER_VERSION}" VERSION_LESS "3.0.0" OR NOT "${CMAKE_C_COMPILER_VERSION}" VERSION_LESS "4.0.0")
			message (WARNING
				"LibraryLink DLL generation requires the -mno-cygwin compiler flag, which is not supported by gcc ${CMAKE_C_COMPILER_VERSION}."
				" Run cmake with options -DCMAKE_CXX_COMPILER=/usr/bin/g++-3.exe -DCMAKE_C_COMPILER=/usr/bin/gcc-3.exe.")
		endif()
	endif()
endmacro(_log_found_variables)

# internal macro returns cache variables that determine search result
macro (_get_cache_variables _CacheVariables)
	set (${_CacheVariables}
		Mathematica_FIND_VERSION
		Mathematica_FIND_VERSION_EXACT
		Mathematica_USE_STATIC_LIBRARIES
		Mathematica_USE_MINIMAL_LIBRARIES
		Mathematica_USE_LIBCXX_LIBRARIES
		Mathematica_SYSTEM_IDS
		Mathematica_CREATION_ID
		Mathematica_ROOT_DIR
		Mathematica_HOST_ROOT_DIR
		Mathematica_MathLink_FIND_VERSION_MAJOR
		Mathematica_MathLink_FIND_VERSION_MINOR
		Mathematica_MathLink_ROOT_DIR
		Mathematica_MathLink_HOST_ROOT_DIR
		Mathematica_WSTP_FIND_VERSION_MAJOR
		Mathematica_WSTP_FIND_VERSION_MINOR
		Mathematica_WSTP_ROOT_DIR
		Mathematica_WSTP_HOST_ROOT_DIR
		Mathematica_JLink_PACKAGE_DIR
		Mathematica_MUnit_PACKAGE_FILE
		Mathematica_LibraryLink_PACKAGE_FILE
		Mathematica_CMAKE_MODULE_VERSION)
endmacro()

# internal macro returns cache variables that are dependent on the given variable
macro (_get_dependent_cache_variables _var _outDependentVars)
	# do comparisons with an underscore prefix to prevent CMake from automatically
	# resolving the left and right hand arguments to STREQUAL
	if ("_${_var}" STREQUAL "_Mathematica_FIND_VERSION" OR
		"_${_var}" STREQUAL "_Mathematica_FIND_VERSION_EXACT")
		list (APPEND ${_outDependentVars}
			Mathematica_ROOT_DIR Mathematica_HOST_ROOT_DIR Mathematica_VERSION)
		_get_dependent_cache_variables("Mathematica_ROOT_DIR" ${_outDependentVars})
		_get_dependent_cache_variables("Mathematica_HOST_ROOT_DIR" ${_outDependentVars})
	elseif ("_${_var}" STREQUAL "_Mathematica_ROOT_DIR" OR
			"_${_var}" STREQUAL "_Mathematica_SYSTEM_IDS")
		list (APPEND ${_outDependentVars}
			Mathematica_VERSION
			Mathematica_INCLUDE_DIR
			Mathematica_WolframLibrary_VERSION
			Mathematica_WolframLibrary_INCLUDE_DIR
			Mathematica_WolframLibrary_LIBRARY
			Mathematica_KERNEL_HOST_SYSTEM_ID
			Mathematica_MathLink_ROOT_DIR
			Mathematica_WSTP_ROOT_DIR
			Mathematica_KERNEL_BASE_DIR
			Mathematica_KERNEL_USERBASE_DIR)
		_get_dependent_cache_variables("Mathematica_MathLink_ROOT_DIR" ${_outDependentVars})
		_get_dependent_cache_variables("Mathematica_WSTP_ROOT_DIR" ${_outDependentVars})
	elseif ("_${_var}" STREQUAL "_Mathematica_CREATION_ID")
		# all cached Mathematica version variables are dependent on the cached creation ID
		list (APPEND ${_outDependentVars}
			Mathematica_VERSION
			Mathematica_WolframLibrary_VERSION
			Mathematica_MathLink_VERSION
			Mathematica_WSTP_VERSION
			Mathematica_JLink_VERSION
			Mathematica_MUnit_VERSION)
	elseif ("_${_var}" STREQUAL "_Mathematica_HOST_ROOT_DIR" OR
			"_${_var}" STREQUAL "_Mathematica_HOST_SYSTEM_IDS")
		list (APPEND ${_outDependentVars}
			Mathematica_FRONTEND_EXECUTABLE
			Mathematica_KERNEL_EXECUTABLE
			Mathematica_KERNEL_HOST_SYSTEM_ID
			Mathematica_MathLink_HOST_ROOT_DIR
			Mathematica_WSTP_HOST_ROOT_DIR
			Mathematica_KERNEL_BASE_DIR
			Mathematica_KERNEL_USERBASE_DIR
			Mathematica_JLink_PACKAGE_DIR
			Mathematica_MUnit_PACKAGE_FILE
			Mathematica_LibraryLink_PACKAGE_FILE
			Mathematica_JLink_JAVA_EXECUTABLE)
		_get_dependent_cache_variables("Mathematica_MathLink_HOST_ROOT_DIR" ${_outDependentVars})
		_get_dependent_cache_variables("Mathematica_WSTP_HOST_ROOT_DIR" ${_outDependentVars})
		_get_dependent_cache_variables("Mathematica_JLink_PACKAGE_DIR" ${_outDependentVars})
		_get_dependent_cache_variables("Mathematica_MUnit_PACKAGE_FILE" ${_outDependentVars})
	elseif ("_${_var}" STREQUAL "_Mathematica_MathLink_ROOT_DIR")
		list (APPEND ${_outDependentVars}
			Mathematica_MathLink_VERSION
			Mathematica_MathLink_INCLUDE_DIR
			Mathematica_MathLink_LIBRARY)
	elseif ("_${_var}" STREQUAL "_Mathematica_MathLink_HOST_ROOT_DIR")
		list (APPEND ${_outDependentVars}
			Mathematica_MathLink_HOST_INCLUDE_DIR
			Mathematica_MathLink_MPREP_EXECUTABLE)
	elseif ("_${_var}" STREQUAL "_Mathematica_MathLink_FIND_VERSION_MAJOR" OR
			"_${_var}" STREQUAL "_Mathematica_MathLink_FIND_VERSION_MINOR")
		list (APPEND ${_outDependentVars}
			Mathematica_MathLink_VERSION
			Mathematica_MathLink_INCLUDE_DIR
			Mathematica_MathLink_LIBRARY
			Mathematica_MathLink_HOST_INCLUDE_DIR
			Mathematica_MathLink_MPREP_EXECUTABLE)
	elseif ("_${_var}" STREQUAL "_Mathematica_WSTP_ROOT_DIR")
		list (APPEND ${_outDependentVars}
			Mathematica_WSTP_VERSION
			Mathematica_WSTP_INCLUDE_DIR
			Mathematica_WSTP_LIBRARY)
	elseif ("_${_var}" STREQUAL "_Mathematica_WSTP_HOST_ROOT_DIR")
		list (APPEND ${_outDependentVars}
			Mathematica_WSTP_HOST_INCLUDE_DIR
			Mathematica_WSTP_WSPREP_EXECUTABLE)
	elseif ("_${_var}" STREQUAL "_Mathematica_WSTP_FIND_VERSION_MAJOR" OR
			"_${_var}" STREQUAL "_Mathematica_WSTP_FIND_VERSION_MINOR")
		list (APPEND ${_outDependentVars}
			Mathematica_WSTP_VERSION
			Mathematica_WSTP_INCLUDE_DIR
			Mathematica_WSTP_LIBRARY
			Mathematica_WSTP_HOST_INCLUDE_DIR
			Mathematica_WSTP_WSPREP_EXECUTABLE)
	elseif ("_${_var}" STREQUAL "_Mathematica_USE_STATIC_LIBRARIES")
		list (APPEND ${_outDependentVars}
			Mathematica_WolframLibrary_LIBRARY
			Mathematica_MathLink_LIBRARY
			Mathematica_MathLink_INCLUDE_DIR
			Mathematica_MathLink_HOST_INCLUDE_DIR
			Mathematica_WSTP_LIBRARY
			Mathematica_WSTP_INCLUDE_DIR
			Mathematica_WSTP_HOST_INCLUDE_DIR)
	elseif ("_${_var}" STREQUAL "_Mathematica_USE_MINIMAL_LIBRARIES")
		list (APPEND ${_outDependentVars}
			Mathematica_WolframLibrary_LIBRARY)
	elseif ("_${_var}" STREQUAL "_Mathematica_USE_LIBCXX_LIBRARIES")
		list (APPEND ${_outDependentVars}
			Mathematica_MathLink_LIBRARY
			Mathematica_WSTP_LIBRARY)
	elseif ("_${_var}" STREQUAL "_Mathematica_JLink_PACKAGE_DIR")
		list (APPEND ${_outDependentVars}
			Mathematica_JLink_VERSION Mathematica_JLink_RUNTIME_LIBRARY)
	elseif ("_${_var}" STREQUAL "_Mathematica_MUnit_PACKAGE_FILE")
		list (APPEND ${_outDependentVars}
			Mathematica_MUnit_VERSION)
	endif()
endmacro(_get_dependent_cache_variables)

# internal macro to cleanup outdated cache variables
macro (_cleanup_cache)
	_get_cache_variables(_CacheVariables)
	set (_vars_to_clean "")
	foreach (_CacheVariable IN LISTS _CacheVariables)
		get_property(_cacheVariableType CACHE "${_CacheVariable}" PROPERTY TYPE)
		if (DEFINED ${_CacheVariable} AND DEFINED ${_CacheVariable}_LAST)
			if (NOT "${${_CacheVariable}}" STREQUAL "${${_CacheVariable}_LAST}")
				# search var has changed
				_get_dependent_cache_variables(${_CacheVariable} _vars_to_clean)
				if (Mathematica_DEBUG)
					message (STATUS "${_CacheVariable} changed from ${${_CacheVariable}_LAST} to ${${_CacheVariable}}")
				endif()
			elseif ("${_cacheVariableType}" MATCHES "PATH" AND
				NOT "${${_CacheVariable}}" MATCHES "-NOTFOUND$" AND
				NOT EXISTS "${${_CacheVariable}}")
				# original var path no longer exists
				list (APPEND _vars_to_clean "${_CacheVariable}")
				_get_dependent_cache_variables(${_CacheVariable} _vars_to_clean)
				if (Mathematica_DEBUG)
					message (STATUS "${_CacheVariable} path ${${_CacheVariable}} no longer exists")
				endif()
			elseif ("${_cacheVariableType}" MATCHES "PATH" AND
				EXISTS "${${_CacheVariable}}" AND
				"${${_CacheVariable}}" IS_NEWER_THAN "${CMAKE_CACHEFILE_DIR}/CMakeCache.txt")
				# search var path has changed
				_get_dependent_cache_variables(${_CacheVariable} _vars_to_clean)
				if (Mathematica_DEBUG)
					message (STATUS "${_CacheVariable} path ${${_CacheVariable}} modified since last CMake run")
				endif()
			endif()
		elseif (DEFINED ${_CacheVariable} OR DEFINED ${_CacheVariable}_LAST)
			# search var presence changed
			_get_dependent_cache_variables(${_CacheVariable} _vars_to_clean)
			if (Mathematica_DEBUG)
				message (STATUS "${_CacheVariable} presence changed")
			endif()
		endif()
	endforeach()
	if (_vars_to_clean)
		list (REMOVE_DUPLICATES _vars_to_clean)
		message (STATUS "Mathematica environment changed, restart search ...")
		if (Mathematica_DEBUG)
			message (STATUS "Unset ${_vars_to_clean}")
		endif()
		foreach (_CacheVariable IN LISTS _vars_to_clean)
			unset(${_CacheVariable} CACHE)
			unset(${_CacheVariable})
		endforeach()
	endif()
endmacro()

# internal macro to update cache variables
macro (_update_cache)
	mark_as_advanced(
		Mathematica_INCLUDE_DIR
		Mathematica_KERNEL_EXECUTABLE
		Mathematica_FRONTEND_EXECUTABLE
		Mathematica_WolframLibrary_INCLUDE_DIR
		Mathematica_WolframLibrary_LIBRARY
		Mathematica_MathLink_INCLUDE_DIR
		Mathematica_MathLink_LIBRARY
		Mathematica_MathLink_HOST_INCLUDE_DIR
		Mathematica_MathLink_MPREP_EXECUTABLE
		Mathematica_WSTP_INCLUDE_DIR
		Mathematica_WSTP_LIBRARY
		Mathematica_WSTP_HOST_INCLUDE_DIR
		Mathematica_WSTP_WSPREP_EXECUTABLE
		Mathematica_KERNEL_HOST_SYSTEM_ID
		Mathematica_KERNEL_BASE_DIR
		Mathematica_KERNEL_USERBASE_DIR
		Mathematica_MUnit_PACKAGE_FILE
		Mathematica_LibraryLink_PACKAGE_FILE
		Mathematica_JLink_RUNTIME_LIBRARY
		Mathematica_JLink_JAVA_EXECUTABLE
	)
	_get_cache_variables(_CacheVariables)
	foreach (_CacheVariable IN LISTS _CacheVariables)
		if (DEFINED ${_CacheVariable})
			set (${_CacheVariable}_LAST ${${_CacheVariable}}
				CACHE INTERNAL "Last value of ${_CacheVariable}." FORCE)
		else()
			unset(${_CacheVariable}_LAST CACHE)
		endif()
	endforeach()
endmacro()

# internal macro to return variables that need to exist in order for component
# to be considered found successfully
macro (_get_required_vars _component _outVars)
	if ("${_component}" STREQUAL "Mathematica")
		set (${_outVars}
			Mathematica_ROOT_DIR
			Mathematica_KERNEL_EXECUTABLE Mathematica_FRONTEND_EXECUTABLE)
	elseif ("${_component}" STREQUAL "MathLink")
		set (${_outVars}
			Mathematica_MathLink_LIBRARY Mathematica_MathLink_INCLUDE_DIR)
	elseif ("${_component}" STREQUAL "WSTP")
		set (${_outVars}
			Mathematica_WSTP_LIBRARY Mathematica_WSTP_INCLUDE_DIR)
	elseif ("${_component}" STREQUAL "WolframLibrary")
		set (${_outVars}
			Mathematica_WolframLibrary_LIBRARY Mathematica_WolframLibrary_INCLUDE_DIR)
	elseif ("${_component}" STREQUAL "JLink")
		set (${_outVars}
			Mathematica_JLink_PACKAGE_DIR Mathematica_JLink_JAR_FILE)
	elseif ("${_component}" STREQUAL "MUnit")
		set (${_outVars}
			Mathematica_MUnit_PACKAGE_DIR)
	endif()
endmacro()

macro (_get_components_to_find _outComponents)
	if (Mathematica_FIND_COMPONENTS)
		list (APPEND ${_outComponents} ${Mathematica_FIND_COMPONENTS})
	else()
		if (DEFINED Mathematica_FIND_VERSION_MAJOR)
			set (_versionMajor "${Mathematica_FIND_VERSION_MAJOR}")
		elseif (DEFINED Mathematica_VERSION_MAJOR)
			set (_versionMajor "${Mathematica_VERSION_MAJOR}")
		else()
			set (_versionMajor "")
		endif()
		if (_versionMajor)
			if (_versionMajor GREATER 9)
				list (APPEND ${_outComponents} "WSTP" "MathLink" "WolframLibrary" "JLink" "MUnit")
			elseif (_versionMajor GREATER 7)
				list (APPEND ${_outComponents} "MathLink" "WolframLibrary" "JLink" "MUnit")
			else()
				list (APPEND ${_outComponents} "MathLink" "JLink" "MUnit")
			endif()
		else()
			list (APPEND ${_outComponents} "WSTP" "MathLink" "WolframLibrary" "JLink" "MUnit")
		endif()
	endif()
	list (REMOVE_DUPLICATES ${_outComponents})
endmacro()

# internal macro to handle the QUIETLY and REQUIRED arguments and set *_FOUND variables
macro (_setup_found_variables)
	# determine required Mathematica components
	_get_required_vars("Mathematica" _requiredVars)
	_get_components_to_find(_components)
	foreach(_component IN LISTS _components)
		_get_required_vars(${_component} _requiredComponentVars)
		find_package_handle_standard_args(
			Mathematica_${_component}
			REQUIRED_VARS ${_requiredComponentVars}
			VERSION_VAR Mathematica_${_component}_VERSION)
		string(TOUPPER ${_component} _UpperCaseComponent)
		# find_package_handle_standard_args only sets upper case _FOUND variable
		set (Mathematica_${_component}_FOUND ${MATHEMATICA_${_UpperCaseComponent}_FOUND})
		if (Mathematica_FIND_REQUIRED_${_component})
			list (APPEND _requiredVars ${_requiredComponentVars} )
		endif()
	endforeach()
	find_package_handle_standard_args(
		Mathematica
		REQUIRED_VARS ${_requiredVars}
		VERSION_VAR Mathematica_VERSION)
	# find_package_handle_standard_args only sets upper case _FOUND variable
	set (Mathematica_FOUND ${MATHEMATICA_FOUND})
endmacro()

# internal macro that searches for requested components
macro (_find_components)
	_get_components_to_find(_components)
	foreach(_component IN LISTS _components)
		if ("${_component}" STREQUAL "MathLink")
			_find_mathlink()
			_setup_mathlink_version_variables()
			_setup_mathlink_library_variables()
		elseif ("${_component}" STREQUAL "WSTP")
			_find_wstp()
			_setup_wstp_version_variables()
			_setup_wstp_library_variables()
		elseif ("${_component}" STREQUAL "WolframLibrary")
			_find_wolframlibrary()
			_setup_wolframlibrary_version_variables()
			_setup_wolframlibrary_library_variables()
			_find_librarylink_package()
		elseif ("${_component}" STREQUAL "JLink")
			_find_jlink()
			_setup_jlink_version_variables()
		elseif ("${_component}" STREQUAL "MUnit")
			_find_munit_package()
			_setup_munit_package_version_variables()
		else()
			message (FATAL_ERROR "Unknown Mathematica component ${_component}")
		endif()
	endforeach()
	list (REMOVE_DUPLICATES Mathematica_INCLUDE_DIRS)
	list (REMOVE_DUPLICATES Mathematica_LIBRARIES)
	list (REMOVE_DUPLICATES Mathematica_LIBRARY_DIRS)
	list (REMOVE_DUPLICATES Mathematica_RUNTIME_LIBRARY_DIRS)
	list (REMOVE_DUPLICATES Mathematica_RUNTIME_LIBRARY_DIRS_DEBUG)
endmacro()

# internal helper function to compute the install name of a shared library under Mac OS X
macro (_get_install_name _libraryPath _libraryInstallName _libraryAbsPath)
	if (APPLE)
		set (${_libraryInstallName} "")
		set (${_libraryAbsPath} "")
		if (IS_DIRECTORY "${_libraryPath}")
			# framework folder
			get_filename_component(_name "${_libraryPath}" NAME_WE)
			set (_path "${_libraryPath}/${_name}")
		else()
			set (_path "${_libraryPath}")
		endif()
		if (EXISTS "${_path}")
			find_program(Mathematica_OTOOL_EXECUTABLE "otool")
			mark_as_advanced(Mathematica_OTOOL_EXECUTABLE)
			get_filename_component(${_libraryAbsPath} ${_path} ABSOLUTE)
			set (_otoolOutput "")
			if (Mathematica_OTOOL_EXECUTABLE)
				execute_process(
					COMMAND "${Mathematica_OTOOL_EXECUTABLE}" "-D" "-X" "${${_libraryAbsPath}}" TIMEOUT 5
					OUTPUT_VARIABLE _otoolOutput OUTPUT_STRIP_TRAILING_WHITESPACE)
				# install name is in last line of otool output
				string (REPLACE "\n" ";" _otoolOutput "${_otoolOutput}")
			endif()
			if (_otoolOutput)
				list (GET _otoolOutput -1 ${_libraryInstallName})
			else()
				set (${_libraryInstallName} "")
			endif()
		endif()
	endif()
endmacro()

# FindMathematica "main" starts here
_setup_findmathematica_options()
_log_used_variables()
_setup_mathematica_systemIDs()
_setup_mathematica_creationID()
if (DEFINED Mathematica_SYSTEM_IDS_LAST)
	# not the initial find invocation
	_cleanup_cache()
endif()
_setup_mathematica_base_directory()
_setup_mathematica_userbase_directory()
_find_mathematica()
_setup_mathematica_version_variables()

# now setup public functions based on found components

# public function to convert a CMake string to a Mathematica string
function (Mathematica_TO_NATIVE_STRING _inStr _outStr)
	string (REPLACE "\\" "\\\\" _str ${_inStr})
	string (REPLACE "\"" "\\\"" _str ${_str})
	set (${_outStr} "\"${_str}\"" PARENT_SCOPE)
endfunction()

# public function to convert a CMake list to a Mathematica list
function (Mathematica_TO_NATIVE_LIST _outList)
	set (_list "{")
	foreach (_elem ${ARGN})
		Mathematica_TO_NATIVE_STRING(${_elem} _elemStr)
		if ("${_list}" STREQUAL "{")
			set (_list "{${_elemStr}")
		else()
			set (_list "${_list},${_elemStr}")
		endif()
	endforeach()
	set (${_outList} "${_list}}" PARENT_SCOPE)
endfunction()

# public function to convert CMake paths to Mathematica paths
function (Mathematica_TO_NATIVE_PATH _inPathStr _outPathStr)
	list (LENGTH _inPathStr _len)
	if (_len EQUAL 0)
		set (${_outPathStr} "" PARENT_SCOPE)
	elseif (_len EQUAL 1)
		_to_native_path("${_inPathStr}" _nativePath)
		Mathematica_TO_NATIVE_STRING("${_nativePath}" _pathMma)
		set (${_outPathStr} "${_pathMma}" PARENT_SCOPE)
	else()
		set (_lastDir "")
		set (_names "")
		set (_nativePathsMma "")
		set (_hasMapPaths FALSE)
		set (_requiresList FALSE)
		foreach (_path IN LISTS _inPathStr ITEMS "")
			get_filename_component(_dir "${_path}" DIRECTORY)
			get_filename_component(_name "${_path}" NAME)
			if (_lastDir AND NOT "${_dir}" STREQUAL "${_lastDir}")
				list (LENGTH _names _nameCount)
				if (_nameCount GREATER 1)
					Mathematica_TO_NATIVE_PATH("${_lastDir}" _commonDirMma)
					Mathematica_TO_NATIVE_LIST(_namesMma ${_names})
					set (_code "Map[ToFileName[${_commonDirMma},#]&,${_namesMma}]")
					set (_hasMapPaths TRUE)
				else()
					Mathematica_TO_NATIVE_PATH("${_lastDir}/${_names}" _code)
				endif()
				if (_nativePathsMma)
					set (_nativePathsMma "${_nativePathsMma},${_code}")
					set (_requiresList TRUE)
				else()
					set (_nativePathsMma "${_code}")
				endif()
				set (_names "")
			endif()
			set (_lastDir "${_dir}")
			list (APPEND _names "${_name}")
		endforeach()
		if (_requiresList AND _hasMapPaths)
			set (_nativePathsMma "Flatten[{${_nativePathsMma}}]")
		elseif (_requiresList)
			set (_nativePathsMma "{${_nativePathsMma}}")
		endif()
		set (${_outPathStr} "${_nativePathsMma}" PARENT_SCOPE)
	endif()
endfunction()

# public function to initialize Mathematica test properties
function (Mathematica_SET_TESTS_PROPERTIES)
	_select_configuration_run_time_dirs(_configRuntimeDirs)
	_get_host_library_search_path_envvars(_envVars)
	foreach (_envVar IN LISTS _envVars)
		if (DEFINED ENV{${_envVar}})
			file (TO_CMAKE_PATH "$ENV{${_envVar}}" _envRuntimeDirs)
			# prepend Mathematica runtime directories to system ones
			set (_runtimeDirs ${_configRuntimeDirs} ${_envRuntimeDirs})
		else()
			set (_runtimeDirs ${_configRuntimeDirs})
		endif()
		if (_runtimeDirs)
			list (REMOVE_DUPLICATES _runtimeDirs)
			if (CYGWIN)
				# CYGWIN path list requires UNIX syntax
				_to_cmake_path_list(_nativeRuntimeDirs ${_runtimeDirs})
			else()
				_to_native_path_list(_nativeRuntimeDirs ${_runtimeDirs})
			endif()
			foreach (_testName ${ARGV})
				if ("${_testName}" STREQUAL "PROPERTIES")
					break()
				endif()
				set_property (TEST ${_testName} APPEND PROPERTY
					ENVIRONMENT "${_envVar}=${_nativeRuntimeDirs}" )
			endforeach()
		endif()
	endforeach()
	set (_haveProperties False)
	foreach (_testName IN ITEMS ${ARGV})
		if ("${_testName}" STREQUAL "PROPERTIES")
			set (_haveProperties True)
			break()
		endif()
		set_property (TEST ${_testName} APPEND PROPERTY LABELS "Mathematica")
	endforeach()
	if (_haveProperties)
		set_tests_properties (${ARGV})
	endif()
endfunction(Mathematica_SET_TESTS_PROPERTIES)

# internal macro to return test driver for host platform
function (_add_test_driver _cmdVar _testName _inputVar _inputFileVar)
	if (CMAKE_HOST_UNIX)
		set (_testDriver "${Mathematica_CMAKE_MODULE_DIR}/FindMathematicaTestDriver.sh")
	elseif (CMAKE_HOST_WIN32)
		set (_testDriver "${Mathematica_CMAKE_MODULE_DIR}/FindMathematicaTestDriver.cmd")
	endif()
	if (NOT EXISTS "${_testDriver}")
		message (FATAL_ERROR "FindMathematica test driver script ${_testDriver} is missing.")
	endif()
	_make_file_executable(${_testDriver})
	if (CYGWIN)
		_to_cmake_path("${_testDriver}" _testDriver)
	else()
		_to_native_path("${_testDriver}" _testDriver)
	endif()
	list (APPEND ${_cmdVar} "${_testDriver}" "${_testName}" "$<CONFIGURATION>")
	if (DEFINED ${_inputVar})
		list (APPEND ${_cmdVar} "input" "${${_inputVar}}")
	elseif (DEFINED ${_inputFileVar})
		list (APPEND ${_cmdVar} "inputfile" "${${_inputFileVar}}")
	else()
		list (APPEND ${_cmdVar} "noinput")
	endif()
	set (${_cmdVar} ${${_cmdVar}} PARENT_SCOPE)
endfunction()

# internal macro to add platform specific executable launch prefix
macro (_add_launch_prefix _cmdVar _systemIDVar)
	if (DEFINED ${_systemIDVar})
		if (CMAKE_HOST_APPLE)
			if (NOT "${${_systemIDVar}}" STREQUAL "${Mathematica_HOST_SYSTEM_ID}")
				# under Mac OS X, run appropriate target architecture of executable universal binary
				# by using the the /usr/bin/arch tool which is available since Leopard
				# (Mac OS X 10.5.0 is Darwin 9.0.0)
				if ("${CMAKE_HOST_SYSTEM_VERSION}" VERSION_LESS "9.0.0")
					message (STATUS "Executable system ID selection of ${${_systemIDVar}} is not supported, running default.")
				elseif ("${${_systemIDVar}}" STREQUAL "MacOSX-x86")
					list (APPEND ${_cmdVar} "/usr/bin/arch" "-i386")
				elseif("${${_systemIDVar}}" STREQUAL "MacOSX-x86-64")
					list (APPEND ${_cmdVar} "/usr/bin/arch" "-x86_64")
				elseif("${${_systemIDVar}}" MATCHES "Darwin|MacOSX")
					list (APPEND ${_cmdVar} "/usr/bin/arch" "-ppc")
				elseif("${${_systemIDVar}}" STREQUAL "Darwin-PowerPC64")
					list (APPEND ${_cmdVar} "/usr/bin/arch" "-ppc64")
				else()
					message (STATUS "Executable system ID ${${_systemIDVar}} is not supported, running default.")
				endif()
			endif()
		endif()
	endif()
endmacro()

# internal macro to set up kernel launch command
macro (_add_kernel_launch_code _cmdVar _systemIDVar _kernelOptionsVar)
	if (CMAKE_HOST_WIN32 OR CYGWIN)
		set (_kernelExecutable "${Mathematica_KERNEL_EXECUTABLE}")
		if (DEFINED ${_systemIDVar})
			# under Windows, run alternate binary for given system ID
			get_filename_component(_kernelName "${_kernelExecutable}" NAME)
			set (_kernelExecutable
				"${Mathematica_HOST_ROOT_DIR}/SystemFiles/Kernel/Binaries/${${_systemIDVar}}/${_kernelName}")
			if (NOT EXISTS "${_kernelExecutable}")
				set (_kernelExecutable "${Mathematica_KERNEL_EXECUTABLE}")
				if (NOT "${_systemIDVar}" STREQUAL "${Mathematica_HOST_SYSTEM_ID}")
					message (STATUS "Kernel executable for ${${_systemIDVar}} is not available, running default ${Mathematica_HOST_SYSTEM_ID}.")
				endif()
			endif()
		endif()
		_to_native_path("${_kernelExecutable}" _kernelExecutable)
		list (APPEND ${_cmdVar} "${_kernelExecutable}")
	elseif (CMAKE_HOST_APPLE)
		_add_launch_prefix(${_cmdVar} ${_systemIDVar})
		_to_native_path("${Mathematica_KERNEL_EXECUTABLE}" _kernelExecutable)
		list (APPEND ${_cmdVar} "${_kernelExecutable}")
	elseif (CMAKE_HOST_UNIX)
		_to_native_path("${Mathematica_KERNEL_EXECUTABLE}" _kernelExecutable)
		list (APPEND ${_cmdVar} "${_kernelExecutable}")
		if (DEFINED ${_systemIDVar})
			if (Mathematica_VERSION)
				if (NOT "${Mathematica_VERSION}" VERSION_LESS "8.0")
					# Mathematica 8 kernel wrapper shell script supports option -SystemID
					list (APPEND ${_cmdVar} "-SystemID" "${${_systemIDVar}}")
				elseif (NOT "${_systemIDVar}" STREQUAL "${Mathematica_HOST_SYSTEM_ID}")
					message (STATUS "Kernel system ID selection of ${${_systemIDVar}} is not supported, running default ${Mathematica_HOST_SYSTEM_ID}.")
				endif()
			endif()
		endif()
	else()
		message (FATAL_ERROR "Unsupported host platform ${CMAKE_HOST_SYSTEM_NAME}")
	endif()
	if (DEFINED ${_kernelOptionsVar})
		list (APPEND ${_cmdVar} ${${_kernelOptionsVar}})
	else()
		list (APPEND ${_cmdVar} "-noinit" "-noprompt")
	endif()
endmacro(_add_kernel_launch_code)

macro (_test_use_tempfile_for_code_segments _codeVar _useTempFileVar)
	set (_codeLength 0)
	set (_codeSegmentCount 1)
	set (_usesReservedChars FALSE)
	foreach (_codeSegment IN LISTS ${_codeVar})
		string (LENGTH "${_codeSegment}" _codeSegmentLength)
		math (EXPR _codeLength "${_codeLength} + ${_codeSegmentLength}")
		if (_codeSegment MATCHES "(Get|Needs|Install|Sequence)\\[[^]]*\\]")
			# start new code segment
			math (EXPR _codeSegmentCount "${_codeSegmentCount} + 1")
		endif()
		if (NOT _usesReservedChars)
			if (_codeSegment MATCHES "[<>|&!%^]")
				set (_usesReservedChars TRUE)
			endif()
		endif()
	endforeach()
	if (CMAKE_HOST_WIN32 AND (_usesReservedChars OR _codeLength GREATER 1000 OR _codeSegmentCount GREATER 3))
		# under Windows XP or later cmd.exe has a command line length limit of 8191 characters.
		# we do not use inline statements if the approximate command line length
		# might exceed that limit or there are too many individual arguments.
		# we write the inline statements to a temporary script instead
		set (${_useTempFileVar} TRUE)
	elseif (CMAKE_HOST_UNIX AND (_codeLength GREATER 10000 OR _codeSegmentCount GREATER 10))
		# for UNIX use a temp file if command line becomes confusing
		set (${_useTempFileVar} TRUE)
	else()
		set (${_useTempFileVar} FALSE)
	endif()
endmacro()

macro (_code_segments_to_compound_expressions _codeVar _codeSegments)
	# collect all CODE sections into CompoundExpressions
	set (${_codeSegments} "")
	set (_currentCodeSegment "")
	set (_currentCodeSegmentCompound False)
	foreach (_codeSegment IN LISTS ${_codeVar} ITEMS "Sequence[]")
		if (_codeSegment MATCHES "\n")
			# remove indentation with tabs
			string (REGEX REPLACE "\t+" "" _codeSegment "${_codeSegment}")
			# separate multiple lines via commas
			string (REPLACE "\n" "," _codeSegment "${_codeSegment}")
		endif()
		# prevent CMake from interpreting ; as a list separator
		string (REPLACE ";" "\\;" _codeSegment "${_codeSegment}")
		if (_currentCodeSegment)
			if (NOT _codeSegment STREQUAL "Sequence[]")
				set (_currentCodeSegmentCompound True)
				set (_currentCodeSegment "${_currentCodeSegment},${_codeSegment}")
			endif()
		else()
			set (_currentCodeSegment "${_codeSegment}")
		endif()
		# flush current CompoundExpression when a Get[...], Needs[...] or Install[...]
		# expression is encountered, so that new context definitions become effective
		# immediately for subsequent commands
		# Sequence[] can be used to explicitly flush the current CompoundExpression
		if (_codeSegment MATCHES "(Get|Needs|Install|Sequence)\\[[^]]*\\]")
			if (_currentCodeSegmentCompound OR
				(CMAKE_HOST_WIN32 AND NOT _currentCodeSegment MATCHES " "))
				# note that the blanks around the CompoundExpression argument below are necessary
				# to force CMake to do proper cmd.exe quoting of the resulting parameter under Windows
				# (a comma in the parameter may be misinterpreted as a separator otherwise)
				list (APPEND ${_codeSegments} "-run" "CompoundExpression[ ${_currentCodeSegment} ]")
			elseif (NOT _currentCodeSegment STREQUAL "Sequence[]")
				# flush single code segment, but only if it is not a NOP
				list (APPEND ${_codeSegments} "-run" "${_currentCodeSegment}")
			endif()
			set (_currentCodeSegment "")
			set (_currentCodeSegmentCompound False)
		endif()
	endforeach()
endmacro(_code_segments_to_compound_expressions)

macro (_code_segments_to_tempfile _codeVar _tempScriptFile)
	# check for use of CMake generator expressions in inline code
	set (_contentsHasGeneratorExpressions FALSE)
	set (_contents "")
	foreach (_codeSegment IN LISTS ${_codeVar})
		string (REPLACE ";" "\\;" _line "${_codeSegment}")
		list (APPEND _contents "${_line}")
		if (NOT _contentsHasGeneratorExpressions)
			if ("${_line}" MATCHES "\\$<.*>")
				set (_contentsHasGeneratorExpressions TRUE)
			endif()
		endif()
	endforeach()
	string (REPLACE ";" "\n" _contents "${_contents}")
	# use script content MD5 as temporary file name
	string (MD5 _scriptName "${_contents}")
	set (_tempScript "${CMAKE_CURRENT_BINARY_DIR}/FindMathematica/${_scriptName}.m")
	file (WRITE "${_tempScript}" "${_contents}")
	if (_contentsHasGeneratorExpressions)
		set (_configNameOrNoneGeneratorExpression "$<$<CONFIG:>:None>$<$<NOT:$<CONFIG:>>:$<CONFIGURATION>>")
		set (_tempConfigScript "${CMAKE_CURRENT_BINARY_DIR}/FindMathematica/${_scriptName}_${_configNameOrNoneGeneratorExpression}.m")
		file (GENERATE OUTPUT "${_tempConfigScript}" INPUT "${_tempScript}")
	else()
		set (_tempConfigScript "${_tempScript}")
	endif()
	set (${_tempScriptFile} "${_tempConfigScript}")
endmacro(_code_segments_to_tempfile)

# internal macro to translate CODE or SCRIPT option to Mathematica launch command
macro (_add_script_or_code _cmdVar _scriptVar _codeVar)
	if (DEFINED ${_codeVar} OR DEFINED ${_scriptVar})
		# start with code to prepend the FindMathematica module directory to the Mathematica $Path
		Mathematica_TO_NATIVE_PATH("${Mathematica_CMAKE_MODULE_DIR}" _cmakeModuleDirMma)
		set (_code "PrependTo[$Path, ${_cmakeModuleDirMma}]")
		# add given inline code statements
		if (DEFINED ${_codeVar})
			list (APPEND _code ${${_codeVar}})
		endif()
		# compute absolute path to given script
		if (DEFINED ${_scriptVar})
			if (IS_ABSOLUTE "${${_scriptVar}}")
				_to_cmake_path("${${_scriptVar}}" _scriptFileAbs)
			else()
				_to_cmake_path("${CMAKE_CURRENT_SOURCE_DIR}/${${_scriptVar}}" _scriptFileAbs)
			endif()
		endif()
		if (NOT DEFINED ${_scriptVar})
			# no given script, quit kernel explicitly unless last code statement already does it
			list (GET _code -1 _lastStatement)
			if (NOT _lastStatement MATCHES "^(Quit|Exit)\\[")
				list (APPEND _code "Quit[]")
			endif()
		elseif ("${Mathematica_VERSION}" VERSION_LESS "10.0")
			# Although the -script option is supported since Mathematica 8, under Mathematica 9
			# using the -script option does not work as expected, if it is preceded by multiple inline
			# Mathematica commands using the -run option.
			# Thus we use the Get function instead, which should work with all versions.
			# According to http://reference.wolfram.com/language/tutorial/WolframLanguageScripts.html
			# running the kernel with the -script option is equivalent to reading the file using the Get function
			# with a single difference: after the last command in the file is evaluated, the kernel terminates
			Mathematica_TO_NATIVE_PATH("${_scriptFileAbs}" _scriptFileMma)
			list (APPEND _code "Get[${_scriptFileMma}]" "Quit[]")
		endif()
		# convert resulting code to kernel inline code segments or if necessary to a temporary script file
		_test_use_tempfile_for_code_segments(_code _useTempFile)
		if (_useTempFile)
			_code_segments_to_tempfile(_code _tempScriptFile)
			Mathematica_TO_NATIVE_PATH("${_tempScriptFile}" _tempScriptFileMma)
			list (APPEND ${_cmdVar} "-run" "Get[${_tempScriptFileMma}]")
		else()
			_code_segments_to_compound_expressions(_code _codeSegments)
			list (APPEND ${_cmdVar} ${_codeSegments})
		endif()
		# finally, run given script with -script option if using Mathematica 10 or later
		if (DEFINED ${_scriptVar})
			if (NOT "${Mathematica_VERSION}" VERSION_LESS "10.0")
				list (APPEND ${_cmdVar} "-script" "${_scriptFileAbs}")
				# after the last command in the script file is evaluated, the kernel terminates automatically
			endif()
		endif()
	endif()
endmacro(_add_script_or_code)

# internal macro to set up linkmode launch command
macro (_add_linkmode_launch_code _cmdVar _protocolKind _systemIDVar _kernelOptionsVar _linkProtocolVar _scriptVar _codeVar)
	list (APPEND ${_cmdVar} "-linkmode" "launch")
	if (DEFINED ${_linkProtocolVar})
		list (APPEND ${_cmdVar} "-linkprotocol" "${${_linkProtocolVar}}")
	endif()
	list (APPEND ${_cmdVar} "-linkname")
	if (UNIX AND NOT CYGWIN)
		# UNIX (except for Cygwin) requires quoted link name path and -mathlink or -wstp
		set (_kernelLaunchArgs "")
		_add_kernel_launch_code(_kernelLaunchArgs ${_systemIDVar} ${_kernelOptionsVar})
		_add_script_or_code(_kernelLaunchArgs ${_scriptVar} ${_codeVar})
		_list_to_cmd_str(_kernelLaunchStr ${_kernelLaunchArgs})
		list (APPEND ${_cmdVar} "${_kernelLaunchStr} ${_protocolKind}")
	else ()
		_add_kernel_launch_code(${_cmdVar} ${_systemIDVar} ${_kernelOptionsVar})
	endif()
endmacro()

if (Mathematica_KERNEL_EXECUTABLE)

# public function for executing Mathematica code file at configuration time
function (Mathematica_EXECUTE)
	set(_options "")
	list(APPEND _options CACHE)
	set(_oneValueArgs
		SCRIPT SYSTEM_ID
		INPUT_FILE OUTPUT_FILE ERROR_FILE
		RESULT_VARIABLE OUTPUT_VARIABLE ERROR_VARIABLE
		TIMEOUT DOC)
	set(_multiValueArgs CODE KERNEL_OPTIONS)
	cmake_parse_arguments(_option "${_options}" "${_oneValueArgs}" "${_multiValueArgs}" ${ARGN})
	if(_option_UNPARSED_ARGUMENTS)
		message (FATAL_ERROR "Unknown keywords: ${_option_UNPARSED_ARGUMENTS}")
	elseif (NOT _option_CODE AND NOT _option_SCRIPT)
		message (FATAL_ERROR "Either the keyword CODE or SCRIPT must be present.")
	endif()
	if (_option_CACHE AND _option_OUTPUT_VARIABLE)
		if (DEFINED "${_option_OUTPUT_VARIABLE}")
			set (_var "${${_option_OUTPUT_VARIABLE}}")
			if (_var AND NOT "${_var}" MATCHES "\\$Failed|\\$Aborted|Mathematica cannot find a valid password")
				# use result from cache if is not a false constant, $Failed, $Aborted or not properly registered
				return()
			endif()
		endif()
	endif()
	set (_cmd COMMAND)
	_add_kernel_launch_code(_cmd _option_SYSTEM_ID _option_KERNEL_OPTIONS)
	_add_script_or_code(_cmd _option_SCRIPT _option_CODE)
	if (_option_CODE)
		list (APPEND _cmd OUTPUT_STRIP_TRAILING_WHITESPACE)
		list (APPEND _cmd ERROR_STRIP_TRAILING_WHITESPACE)
	endif()
	foreach (_key IN LISTS _oneValueArgs)
		set (_value "_option_${_key}")
		if (DEFINED ${_value})
			if (_key MATCHES "_VARIABLE$")
				list (APPEND _cmd ${_key} "${${_value}}")
				list (APPEND _variables "${${_value}}")
			elseif (NOT _key MATCHES "SCRIPT|CODE|SYSTEM_ID|DOC")
				list (APPEND _cmd ${_key} "${${_value}}")
			endif()
		endif()
	endforeach()
	list (APPEND _cmd WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
	if (Mathematica_DEBUG)
		message (STATUS "execute_process: ${_cmd}")
	endif()
	execute_process (${_cmd})
	# put result to cache
	if (_option_OUTPUT_VARIABLE)
		# if Mathematica is not registered properly, exit with a fatal error
		if ("${${_option_OUTPUT_VARIABLE}}" MATCHES "Mathematica cannot find a valid password")
			message (FATAL_ERROR "${${_option_OUTPUT_VARIABLE}}")
		endif()
	endif()
	if (_option_CACHE AND _option_OUTPUT_VARIABLE)
		if (NOT _option_DOC)
			set (_option_DOC "Mathematica_EXECUTE kernel output.")
		endif()
		set (${_option_OUTPUT_VARIABLE}
			"${${_option_OUTPUT_VARIABLE}}" CACHE STRING "${_option_DOC}" FORCE)
	endif()
	# propagate variables to parent scope
	foreach (_var IN LISTS _variables)
		if (DEFINED ${_var})
			set (${_var} ${${_var}} PARENT_SCOPE)
		endif()
	endforeach()
endfunction(Mathematica_EXECUTE)

# public function for executing Mathematica code at build time as a standalone target
function (Mathematica_ADD_CUSTOM_TARGET _targetName)
	set(_options ALL)
	set(_oneValueArgs SCRIPT COMMENT SYSTEM_ID)
	set(_multiValueArgs CODE DEPENDS SOURCES KERNEL_OPTIONS)
	cmake_parse_arguments(_option "${_options}" "${_oneValueArgs}" "${_multiValueArgs}" ${ARGN})
	if(_option_UNPARSED_ARGUMENTS)
		message (FATAL_ERROR "Unknown keywords: ${_option_UNPARSED_ARGUMENTS}")
	elseif (NOT _option_CODE AND NOT _option_SCRIPT)
		message (FATAL_ERROR "Either the keyword CODE or SCRIPT must be present.")
	endif()
	set (_cmd "${_targetName}")
	if (_option_ALL)
		list(APPEND _cmd "ALL")
	endif()
	list(APPEND _cmd COMMAND)
	_add_kernel_launch_code(_cmd _option_SYSTEM_ID _option_KERNEL_OPTIONS)
	_add_script_or_code(_cmd _option_SCRIPT _option_CODE)
	if (_option_SCRIPT)
		list (APPEND _option_DEPENDS ${_option_SCRIPT})
	endif()
	if (_option_DEPENDS)
		list (APPEND _cmd DEPENDS ${_option_DEPENDS})
	endif()
	if (_option_COMMENT)
		list(APPEND _cmd COMMENT ${_option_COMMENT})
	endif()
	if (_option_SOURCES)
		list(APPEND _cmd SOURCES ${_option_SOURCES})
	endif()
	list (APPEND _cmd WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}" VERBATIM)
	if (Mathematica_DEBUG)
		message (STATUS "add_custom_target: ${_cmd}")
	endif()
	add_custom_target(${_cmd})
endfunction(Mathematica_ADD_CUSTOM_TARGET)

# public function for executing Mathematica code at build time to produce output files
function (Mathematica_ADD_CUSTOM_COMMAND)
	set(_options PRE_BUILD PRE_LINK POST_BUILD APPEND)
	set(_oneValueArgs SCRIPT COMMENT MAIN_DEPENDENCY TARGET SYSTEM_ID)
	set(_multiValueArgs CODE OUTPUT DEPENDS KERNEL_OPTIONS)
	cmake_parse_arguments(_option "${_options}" "${_oneValueArgs}" "${_multiValueArgs}" ${ARGN})
	if(_option_UNPARSED_ARGUMENTS)
		message (FATAL_ERROR "Unknown keywords: ${_option_UNPARSED_ARGUMENTS}")
	elseif (NOT _option_CODE AND NOT _option_SCRIPT)
		message (FATAL_ERROR "Either the keyword CODE or SCRIPT must be present.")
	elseif (NOT _option_OUTPUT AND NOT _option_TARGET)
		message (FATAL_ERROR "Either the keyword OUTPUT or TARGET must be present.")
	elseif (_option_OUTPUT AND _option_TARGET)
		message (FATAL_ERROR "Keywords OUTPUT and TARGET are mutually exclusive.")
	endif()
	if (_option_OUTPUT)
		set (_cmd OUTPUT ${_option_OUTPUT})
	endif()
	if (_option_TARGET)
		set (_cmd TARGET ${_option_TARGET})
	endif()
	if (_option_PRE_BUILD)
		list(APPEND _cmd PRE_BUILD)
	endif()
	if (_option_PRE_LINK)
		list(APPEND _cmd PRE_LINK)
	endif()
	if (_option_POST_BUILD)
		list(APPEND _cmd POST_BUILD)
	endif()
	list(APPEND _cmd COMMAND)
	_add_kernel_launch_code(_cmd _option_SYSTEM_ID _option_KERNEL_OPTIONS)
	_add_script_or_code(_cmd _option_SCRIPT _option_CODE)
	if (_option_MAIN_DEPENDENCY)
		list(APPEND _cmd MAIN_DEPENDENCY ${_option_MAIN_DEPENDENCY})
	endif()
	if (_option_SCRIPT AND _option_OUTPUT)
		list (APPEND _option_DEPENDS ${_option_SCRIPT})
	endif()
	if (_option_DEPENDS)
		list(APPEND _cmd DEPENDS ${_option_DEPENDS})
	endif()
	if (_option_COMMENT)
		list(APPEND _cmd COMMENT ${_option_COMMENT})
	endif()
	list (APPEND _cmd WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}" VERBATIM)
	if (_option_APPEND)
		list(APPEND _cmd APPEND)
	endif()
	if (Mathematica_DEBUG)
		message (STATUS "add_custom_command: ${_cmd}")
	endif()
	add_custom_command(${_cmd})
endfunction(Mathematica_ADD_CUSTOM_COMMAND)

# public function to simplify testing Mathematica commands
function (Mathematica_ADD_TEST)
	set(_options "")
	set(_oneValueArgs NAME SCRIPT INPUT INPUT_FILE SYSTEM_ID)
	set(_multiValueArgs CODE CONFIGURATIONS COMMAND KERNEL_OPTIONS)
	cmake_parse_arguments(_option "${_options}" "${_oneValueArgs}" "${_multiValueArgs}" ${ARGN})
	if(_option_UNPARSED_ARGUMENTS)
		message (FATAL_ERROR "Unknown keywords: ${_option_UNPARSED_ARGUMENTS}")
	elseif (NOT _option_NAME)
		message (FATAL_ERROR "Mandatory parameter NAME is missing.")
	elseif (NOT _option_CODE AND NOT _option_SCRIPT AND NOT _option_COMMAND)
		message (FATAL_ERROR "Either the keyword CODE, SCRIPT or COMMAND must be present.")
	endif()
	set (_cmd NAME "${_option_NAME}" COMMAND)
	_add_test_driver(_cmd "${_option_NAME}" _option_INPUT _option_INPUT_FILE)
	if (_option_COMMAND)
		_add_launch_prefix(_cmd _option_SYSTEM_ID)
		list (APPEND _cmd ${_option_COMMAND})
	else()
		_add_kernel_launch_code(_cmd _option_SYSTEM_ID _option_KERNEL_OPTIONS)
		_add_script_or_code(_cmd _option_SCRIPT _option_CODE)
	endif()
	if (_option_CONFIGURATIONS)
		list (APPEND _cmd CONFIGURATIONS ${_option_CONFIGURATIONS})
	endif()
	if (Mathematica_DEBUG)
		message (STATUS "add_test: ${_cmd}")
	endif()
	add_test (${_cmd})
endfunction (Mathematica_ADD_TEST)

# public function to add target that runs Mathematica Splice function on template file
function (Mathematica_SPLICE_C_CODE _templateFile)
	get_filename_component(_templateFileBaseName ${_templateFile} NAME_WE)
	get_filename_component(_templateFileName ${_templateFile} NAME)
	get_filename_component(_templateFileAbs ${_templateFile} ABSOLUTE)
	get_filename_component(_templateFileExt ${_templateFileName} EXT)
	set(_options "")
	set(_oneValueArgs "OUTPUT")
	set(_multiValueArgs "")
	cmake_parse_arguments(_option "${_options}" "${_oneValueArgs}" "${_multiValueArgs}" ${ARGN})
	if(_option_UNPARSED_ARGUMENTS)
		message (FATAL_ERROR "Unknown keywords: ${_option_UNPARSED_ARGUMENTS}")
	endif()
	# Mathematica function Splice does not produce output in current working directory
	# Use absolute paths to make it write to the current binary directory
	if (_option_OUTPUT)
		if (IS_ABSOLUTE ${_option_OUTPUT})
			set (_outputFileAbs "${_option_OUTPUT}")
		else()
			set (_outputFileAbs "${CMAKE_CURRENT_BINARY_DIR}/${_option_OUTPUT}")
		endif()
	else()
		set (_outputFileAbs "${CMAKE_CURRENT_BINARY_DIR}/${_templateFileBaseName}.c")
	endif()
	# Always set FormatType option to prevent Splice function from failing with a
	# Splice::splict error if the template file path contains more than one dot character
	string(TOLOWER ${_templateFileExt} _templateFileExt)
	if ("${_templateFileExt}" STREQUAL ".mc")
		set (_formatType "CForm")
	elseif ("${_templateFileExt}" STREQUAL ".mf")
		set (_formatType "FortranForm")
	elseif ("${_templateFileExt}" STREQUAL ".mtex")
		set (_formatType "TeXForm")
	else()
		set (_formatType "Automatic")
	endif()
	get_filename_component(_outputFileName ${_outputFileAbs} NAME)
	Mathematica_TO_NATIVE_PATH("${_templateFileAbs}" _templateFileMma)
	Mathematica_TO_NATIVE_PATH("${_outputFileAbs}" _outputFileMma)
	set (_msg "Splicing Mathematica code in ${_templateFileName} to ${_outputFileName}")
	Mathematica_ADD_CUSTOM_COMMAND(
		CODE "Splice[${_templateFileMma}, ${_outputFileMma}, FormatType->${_formatType}]"
		OUTPUT "${_outputFileAbs}"
		DEPENDS "${_templateFileAbs}"
		COMMENT ${_msg})
	set_source_files_properties(${_outputFileAbs} PROPERTIES GENERATED TRUE LABELS "Mathematica")
endfunction(Mathematica_SPLICE_C_CODE)

# public function to add target that runs Mathematica Encode function on input files
function (Mathematica_ENCODE)
	set(_options "CHECK_TIMESTAMPS")
	set(_oneValueArgs "COMMENT" "KEY" "MACHINE_ID")
	set(_multiValueArgs "OUTPUT")
	cmake_parse_arguments(_option "${_options}" "${_oneValueArgs}" "${_multiValueArgs}" ${ARGN})
	set (_inputFiles ${_option_UNPARSED_ARGUMENTS})
	list (LENGTH _inputFiles _inputFileCount)
	if (_inputFileCount EQUAL 0)
		message (WARNING "No input files to encode given.")
		return()
	endif()
	if (_option_OUTPUT)
		set (_outputFiles ${_option_OUTPUT})
	else()
		# no output option given, write encoded files to CMAKE_CURRENT_BINARY_DIR
		set (_outputFiles ${CMAKE_CURRENT_BINARY_DIR})
	endif()
	list (LENGTH _outputFiles _outputFileCount)
	if (_outputFileCount EQUAL 1 AND _inputFileCount GREATER 1 AND IS_DIRECTORY "${_outputFiles}")
		# OUTPUT option is a single existing directory, write encoded files to it
		set (_outputDir "${_outputFiles}")
		math(EXPR _lastIndex "${_inputFileCount} - 2")
		foreach(_index RANGE ${_lastIndex})
			list (APPEND _outputFiles "${_outputDir}")
		endforeach()
		set (_outputFileCount ${_inputFileCount})
	endif()
	set (_outputFilesAbs "")
	set (_inputFilesAbs "")
	set (_outputDirs "")
	if (_outputFileCount EQUAL _inputFileCount)
		math(EXPR _lastIndex "${_inputFileCount} - 1")
		foreach(_index RANGE ${_lastIndex})
			list (GET _inputFiles ${_index} _inputFile)
			get_filename_component(_inputFileAbs "${_inputFile}" ABSOLUTE)
			list (APPEND _inputFilesAbs "${_inputFileAbs}")
			list (GET _outputFiles ${_index} _outputFile)
			if (IS_DIRECTORY "${_outputFile}")
				file (RELATIVE_PATH _inputFileRel ${CMAKE_CURRENT_SOURCE_DIR} "${_inputFileAbs}")
				if (NOT IS_ABSOLUTE "${_inputFileRel}" AND NOT "${_inputFileRel}" MATCHES "^\\.\\.")
					set (_outputFile "${_outputFile}/${_inputFileRel}")
				else()
					get_filename_component(_inputFileName "${_inputFile}" NAME)
					set (_outputFile "${_outputFile}/${_inputFileName}")
				endif()
			endif()
			if (IS_ABSOLUTE "${_outputFile}")
				list (APPEND _outputFilesAbs "${_outputFile}")
			else()
				list (APPEND _outputFilesAbs "${CMAKE_CURRENT_BINARY_DIR}/${_outputFile}")
			endif()
			get_filename_component(_outputFileDir "${_outputFile}" DIRECTORY)
			if (NOT _outputFileDir STREQUAL "${CMAKE_CURRENT_BINARY_DIR}")
				list (APPEND _outputDirs "${_outputFileDir}")
			endif()
		endforeach()
	else()
		# OUTPUT option must have exactly one entry for each input file
		message (FATAL_ERROR
			"Number of output files (${_outputFileCount}) does not match number of input files (${_inputFileCount}).")
	endif()
	Mathematica_TO_NATIVE_PATH("${_inputFilesAbs}" _inputFilesAbsMma)
	Mathematica_TO_NATIVE_PATH("${_outputFilesAbs}" _outputFilesAbsMma)
	set (_cmdOptionsMma "")
	if (_option_KEY)
		Mathematica_TO_NATIVE_STRING("${_option_KEY}" _keyMma)
		set (_cmdOptionsMma "${_cmdOptionsMma},${_keyMma}")
	endif()
	if (_option_MACHINE_ID)
		Mathematica_TO_NATIVE_STRING("${_option_MACHINE_ID}" _machineIDMma)
		set (_cmdOptionsMma "${_cmdOptionsMma},MachineID->${_machineIDMma}")
	endif()
	if (_option_CHECK_TIMESTAMPS)
		set (_encodeFunc "If[FileType[#2]==None||OrderedQ[{FileDate[#2],FileDate[#1]}],Encode[#1,#2${_cmdOptionsMma}]]&")
	else()
		set (_encodeFunc "Encode[#1,#2${_cmdOptionsMma}]&")
	endif()
	if (_inputFileCount EQUAL 1)
		set (_func "Apply")
	else()
		set (_func "MapThread")
	endif()
	set (_cmds "")
	if (_outputDirs)
		list (SORT _outputDirs)
		list (REMOVE_DUPLICATES _outputDirs)
		Mathematica_TO_NATIVE_PATH("${_outputDirs}" _outputDirsMma)
		list (APPEND _cmds "Quiet[CreateDirectory[${_outputDirsMma}]]")
	endif()
	list (APPEND _cmds "${_func}[${_encodeFunc},{${_inputFilesAbsMma},${_outputFilesAbsMma}}]")
	if (NOT _option_COMMENT)
		if (_inputFileCount EQUAL 1)
			set (_option_COMMENT "Encoding ${_inputFiles}")
		else()
			set (_option_COMMENT "Encoding ${_inputFileCount} Mathematica files")
		endif()
	endif()
	Mathematica_ADD_CUSTOM_COMMAND(
		CODE ${_cmds}
		OUTPUT ${_outputFilesAbs}
		DEPENDS ${_inputFilesAbs}
		COMMENT "${_option_COMMENT}")
	set_source_files_properties(${_outputFilesAbs} PROPERTIES GENERATED TRUE LABELS "Mathematica")
endfunction(Mathematica_ENCODE)

# public function to find Mathematica package
function (Mathematica_FIND_PACKAGE _var _packageName)
	set(_options "")
	set(_oneValueArgs DOC SYSTEM_ID)
	set(_multiValueArgs KERNEL_OPTIONS)
	cmake_parse_arguments(_option "${_options}" "${_oneValueArgs}" "${_multiValueArgs}" ${ARGN})
	if(_option_UNPARSED_ARGUMENTS)
		message (FATAL_ERROR "Unknown keywords: ${_option_UNPARSED_ARGUMENTS}")
	endif()
	# determine MUnit package directory
	Mathematica_TO_NATIVE_STRING("${_packageName}" _packageNameMma)
	if (DEFINED Mathematica_VERSION)
		if ("${Mathematica_VERSION}" VERSION_LESS "7.0")
			# default to using FileNames function
			set (_findPackage "Print[StandardForm[Check[First[FileNames[ContextToFileName[${_packageNameMma}],$Path]],$Failed]]]")
		else()
			# function FindFile available since Mathematica 7
			set (_findPackage "Print[StandardForm[FindFile[${_packageNameMma}]]]")
		endif()
	endif()
	if (NOT _option_DOC)
		set (_option_DOC "Mathematica package file path.")
	endif()
	set (_cmd
		CODE "${_findPackage}"
		OUTPUT_VARIABLE ${_var}
		CACHE DOC "${_option_DOC}"
		TIMEOUT 10)
	if (_option_KERNEL_FLAGS)
		list (APPEND _cmd KERNEL_OPTIONS ${_option_KERNEL_FLAGS})
	endif()
	if (_option_SYSTEM_ID)
		list (APPEND _cmd SYSTEM_ID ${_option_SYSTEM_ID})
	endif()
	# if package file variable already defined, verify package file existence
	if (DEFINED ${_var})
		if (NOT EXISTS "${${_var}}")
			unset(${_var} CACHE)
			unset(${_var})
		endif()
	endif()
	Mathematica_EXECUTE(${_cmd})
	# verify package file existence
	if (DEFINED ${_var})
		if (EXISTS "${${_var}}")
			_to_cmake_path("${${_var}}" ${_var})
		else()
			set (${_var} "${_var}-NOTFOUND")
		endif()
	else()
		set (${_var} "${_var}-NOTFOUND")
	endif()
	set (${_var} "${${_var}}" CACHE FILEPATH "${_option_DOC}" FORCE)
	set (${_var} "${${_var}}" PARENT_SCOPE)
endfunction()

# public function to get root Mathematica package directory from a package file
function (Mathematica_GET_PACKAGE_DIR _var _packageFile)
	_get_supported_systemIDs("${Mathematica_VERSION}" _intermediateDirs)
	list (APPEND _intermediateDirs
		"Kernel" "SystemResources" "SystemFiles" "Binaries"
		"Libraries" "LibraryResources" "Java" "CSource")
	if (NOT EXISTS "${_packageFile}")
		set (${_var} "${_var}-NOTFOUND" PARENT_SCOPE)
		return()
	endif()
	# walk up directory tree until we find package root dir
	set (_packageFileDir "${_packageFile}")
	set (_index 0)
	while (NOT ${_index} EQUAL -1)
		get_filename_component(_packageFileDir "${_packageFileDir}" DIRECTORY)
		get_filename_component(_name "${_packageFileDir}" NAME)
		list (FIND _intermediateDirs "${_name}" _index)
	endwhile()
	set (${_var} ${_packageFileDir} PARENT_SCOPE)
endfunction()

endif (Mathematica_KERNEL_EXECUTABLE)

# re-compute system IDs and base directories, now that we can query the kernel
_setup_mathematica_systemIDs()
_setup_mathematica_creationID()
_setup_mathematica_base_directory()
_setup_mathematica_userbase_directory()

# find Mathematica components
_find_components()
_setup_mathematica_version_variables()
_update_cache()
_setup_found_variables()
_log_found_variables()

# public function for fixing shared library references to dynamic Mathematica runtime libraries under Mac OS X
function (Mathematica_ABSOLUTIZE_LIBRARY_DEPENDENCIES)
	if (APPLE)
		foreach(_target ${ARGV})
			get_target_property(_targetType ${_target} TYPE)
			if (_targetType MATCHES "MODULE_LIBRARY|SHARED_LIBRARY|EXECUTABLE")
				foreach(_library Mathematica_WolframLibrary_LIBRARY Mathematica_MathLink_LIBRARY Mathematica_WSTP_LIBRARY)
					if (DEFINED ${_library})
						_get_install_name("${${_library}}" _libraryInstallName _libraryAbsPath)
						if (_libraryInstallName)
							add_custom_command (TARGET ${_target}
								POST_BUILD COMMAND "${CMAKE_INSTALL_NAME_TOOL}"
									"-change" "${_libraryInstallName}" "${_libraryAbsPath}"
								"$<TARGET_FILE:${_target}>" VERBATIM)
						endif()
					endif()
				endforeach()
			endif()
		endforeach()
	endif()
endfunction()

if (Mathematica_KERNEL_EXECUTABLE AND Mathematica_MathLink_FOUND)

# public function to simplify testing MathLink programs
function (Mathematica_MathLink_ADD_TEST)
	set(_options "")
	set(_oneValueArgs NAME SCRIPT TARGET INPUT INPUT_FILE SYSTEM_ID LINK_PROTOCOL LINK_MODE)
	set(_multiValueArgs CODE CONFIGURATIONS KERNEL_OPTIONS)
	cmake_parse_arguments(_option "${_options}" "${_oneValueArgs}" "${_multiValueArgs}" ${ARGN})
	if(_option_UNPARSED_ARGUMENTS)
		message (FATAL_ERROR "Unknown keywords: ${_option_UNPARSED_ARGUMENTS}")
	elseif (NOT _option_TARGET)
		message (FATAL_ERROR "Mandatory parameter TARGET is missing.")
	elseif (NOT _option_NAME)
		message (FATAL_ERROR "Mandatory parameter NAME is missing.")
	endif()
	if (NOT _option_LINK_MODE)
		if (_option_CODE OR _option_SCRIPT)
			set (_option_LINK_MODE "ParentConnect")
		else()
			set (_option_LINK_MODE "Launch")
		endif()
	endif()
	set (_cmd NAME "${_option_NAME}" COMMAND)
	_add_test_driver(_cmd "${_option_NAME}" _option_INPUT _option_INPUT_FILE)
	if (_option_LINK_MODE MATCHES "^ParentConnect$")
		# run Mathematica kernel and launch MathLink executable as a child process that connects with ParentConnect
		if (CYGWIN OR MSYS)
			get_target_property (_targetFile ${_option_TARGET} LOCATION)
			Mathematica_TO_NATIVE_PATH("${_targetFile}" _installCmdMma)
		else()
			set (_installCmdMma "\"$<TARGET_FILE:${_option_TARGET}>\"")
		endif()
		set (_launch_prefix "")
		_add_launch_prefix(_launch_prefix _option_SYSTEM_ID)
		if (_launch_prefix)
			Mathematica_TO_NATIVE_LIST(_launch_prefixMma ${_launch_prefix})
			set (_installCmdMma
				"StringJoin[StringInsert[${_launch_prefixMma},\" \",-1],StringInsert[${_installCmdMma},\"\\\"\",{1,-1}]]" )
		endif()
		if (_option_LINK_PROTOCOL)
			set (_installCmd "link=Install[${_installCmdMma},LinkProtocol->\"${_option_LINK_PROTOCOL}\"]")
		else()
			set (_installCmd "link=Install[${_installCmdMma}]")
		endif()
		if (_option_CODE)
			list (APPEND _installCmd ${_option_CODE})
		endif()
		if (NOT _option_SCRIPT)
			list (APPEND _installCmd "Uninstall[link]")
		endif()
		_add_kernel_launch_code(_cmd _option_SYSTEM_ID _option_KERNEL_OPTIONS)
		_add_script_or_code(_cmd _option_SCRIPT _installCmd)
	elseif (_option_LINK_MODE MATCHES "^Launch$")
		# run MathLink executable as front-end to Mathematica kernel
		_add_launch_prefix(_cmd _option_SYSTEM_ID)
		list (APPEND _cmd "$<TARGET_FILE:${_option_TARGET}>")
		_add_linkmode_launch_code(_cmd "-mathlink"
			_option_SYSTEM_ID _option_KERNEL_OPTIONS _option_LINK_PROTOCOL
			_option_SCRIPT _option_CODE)
	else()
		message (FATAL_ERROR "Parameter LINK_MODE must be either \"Launch\" or \"ParentConnect\".")
	endif()
	if (_option_CONFIGURATIONS)
		list (APPEND _cmd CONFIGURATIONS ${_option_CONFIGURATIONS})
	endif()
	if (Mathematica_DEBUG)
		message (STATUS "add_test: ${_cmd}")
	endif()
	add_test (${_cmd})
endfunction(Mathematica_MathLink_ADD_TEST)

endif (Mathematica_KERNEL_EXECUTABLE AND Mathematica_MathLink_FOUND)

if (Mathematica_KERNEL_EXECUTABLE AND Mathematica_WSTP_FOUND)

# public function to simplify testing WSTP programs
function (Mathematica_WSTP_ADD_TEST)
	set(_options "")
	set(_oneValueArgs NAME SCRIPT TARGET INPUT INPUT_FILE SYSTEM_ID LINK_PROTOCOL)
	set(_multiValueArgs CODE CONFIGURATIONS KERNEL_OPTIONS)
	cmake_parse_arguments(_option "${_options}" "${_oneValueArgs}" "${_multiValueArgs}" ${ARGN})
	if(_option_UNPARSED_ARGUMENTS)
		message (FATAL_ERROR "Unknown keywords: ${_option_UNPARSED_ARGUMENTS}")
	elseif (NOT _option_TARGET)
		message (FATAL_ERROR "Mandatory parameter TARGET is missing.")
	elseif (NOT _option_NAME)
		message (FATAL_ERROR "Mandatory parameter NAME is missing.")
	endif()
	set (_cmd NAME "${_option_NAME}" COMMAND)
	_add_test_driver(_cmd "${_option_NAME}" _option_INPUT _option_INPUT_FILE)
	if (_option_CODE OR _option_SCRIPT)
		# run Mathematica kernel and install WSTP executable
		if (CYGWIN OR MSYS)
			get_target_property (_targetFile ${_option_TARGET} LOCATION)
			Mathematica_TO_NATIVE_PATH("${_targetFile}" _installCmdMma)
		else()
			set (_installCmdMma "\"$<TARGET_FILE:${_option_TARGET}>\"")
		endif()
		set (_launch_prefix "")
		_add_launch_prefix(_launch_prefix _option_SYSTEM_ID)
		if (_launch_prefix)
			Mathematica_TO_NATIVE_LIST(_launch_prefixMma ${_launch_prefix})
			set (_installCmdMma
				"StringJoin[StringInsert[${_launch_prefixMma},\" \",-1],StringInsert[${_installCmdMma},\"\\\"\",{1,-1}]]" )
		endif()
		if (_option_LINK_PROTOCOL)
			set (_installCmd "link=Install[${_installCmdMma},LinkProtocol->\"${_option_LINK_PROTOCOL}\"]")
		else()
			set (_installCmd "link=Install[${_installCmdMma}]")
		endif()
		if (_option_CODE)
			list (APPEND _installCmd ${_option_CODE})
		endif()
		if (NOT _option_SCRIPT)
			list (APPEND _installCmd "Uninstall[link]")
		endif()
		_add_kernel_launch_code(_cmd _option_SYSTEM_ID _option_KERNEL_OPTIONS)
		_add_script_or_code(_cmd _option_SCRIPT _installCmd)
	else()
		# run WSTP executable as front-end to Mathematica kernel
		_add_launch_prefix(_cmd _option_SYSTEM_ID)
		list (APPEND _cmd "$<TARGET_FILE:${_option_TARGET}>")
		_add_linkmode_launch_code(_cmd "-wstp"
			_option_SYSTEM_ID _option_KERNEL_OPTIONS _option_LINK_PROTOCOL
			_option_SCRIPT _option_CODE)
	endif()
	if (_option_CONFIGURATIONS)
		list (APPEND _cmd CONFIGURATIONS ${_option_CONFIGURATIONS})
	endif()
	if (Mathematica_DEBUG)
		message (STATUS "add_test: ${_cmd}")
	endif()
	add_test (${_cmd})
endfunction(Mathematica_WSTP_ADD_TEST)

endif (Mathematica_KERNEL_EXECUTABLE AND Mathematica_WSTP_FOUND)

if (Mathematica_KERNEL_EXECUTABLE AND Mathematica_WolframLibrary_FOUND)

# public function to add target that creates C code from Mathematica code
function (Mathematica_GENERATE_C_CODE _packageFile)
	get_filename_component(_packageFileBaseName ${_packageFile} NAME_WE)
	get_filename_component(_packageFileName ${_packageFile} NAME)
	get_filename_component(_packageFileAbs ${_packageFile} ABSOLUTE)
	set(_options "")
	set(_oneValueArgs "OUTPUT")
	set(_multiValueArgs "DEPENDS")
	cmake_parse_arguments(_option "${_options}" "${_oneValueArgs}" "${_multiValueArgs}" ${ARGN})
	if(_option_UNPARSED_ARGUMENTS)
		message (FATAL_ERROR "Unknown keywords: ${_option_UNPARSED_ARGUMENTS}")
	endif()
	if (_option_OUTPUT)
		set (_cSource "${_option_OUTPUT}")
		get_filename_component(_cHeaderBaseName ${_cSource} NAME_WE)
		set (_cHeader "${_cHeaderBaseName}.h")
	else()
		set (_cSource "${_packageFileName}.c")
		set (_cHeader "${_packageFileName}.h")
		set (_cHeaderBaseName "${_packageFileName}")
	endif()
	Mathematica_TO_NATIVE_PATH(${_packageFileAbs} _packageFileAbsMma)
	Mathematica_TO_NATIVE_PATH(${_cSource} _cSourceMma)
	Mathematica_TO_NATIVE_PATH(${_cHeader} _cHeaderMma)
	Mathematica_TO_NATIVE_STRING(${_cHeaderBaseName} _cHeaderBaseNameMma)
	Mathematica_TO_NATIVE_STRING(${_packageFileBaseName} _packageFileBaseNameMma)
	string (REGEX REPLACE "\n|\t" "" _codeGenerate
		"Module[{functions=Get[${_packageFileAbsMma}]},
			If[ListQ[functions],
				CompoundExpression[
					CCodeGenerate[Sequence@@functions,${_cSourceMma},
						\"CodeTarget\"->\"WolframRTL\",
						\"HeaderName\"->${_cHeaderBaseNameMma},
						\"LifeCycleFunctionNames\"->${_packageFileBaseNameMma}],
					CCodeGenerate[Sequence@@functions,${_cHeaderMma},
						\"CodeTarget\"->\"WolframRTLHeader\",
						\"LifeCycleFunctionNames\"->${_packageFileBaseNameMma}]
				]
			]
		]")
	list (INSERT _codeGenerate 0 "Needs[\"CCodeGenerator`\"]")
	set (_msg "Generating source ${_cSource} and header ${_cHeader} from ${_packageFile}")
	list (INSERT _option_DEPENDS 0 "${_packageFileAbs}")
	Mathematica_ADD_CUSTOM_COMMAND(
		OUTPUT "${_cSource}" "${_cHeader}"
		CODE ${_codeGenerate}
		DEPENDS ${_option_DEPENDS}
		COMMENT "${_msg}")
	set_source_files_properties("${_cSource}" "${_cHeader}"
		PROPERTIES GENERATED TRUE LABELS "Mathematica")
endfunction(Mathematica_GENERATE_C_CODE)

# public function to simplify testing WolframLibrary targets
function (Mathematica_WolframLibrary_ADD_TEST)
	set(_options "")
	set(_oneValueArgs NAME SCRIPT TARGET INPUT INPUT_FILE SYSTEM_ID)
	set(_multiValueArgs CODE CONFIGURATIONS KERNEL_OPTIONS)
	cmake_parse_arguments(_option "${_options}" "${_oneValueArgs}" "${_multiValueArgs}" ${ARGN})
	if(_option_UNPARSED_ARGUMENTS)
		message (FATAL_ERROR "Unknown keywords: ${_option_UNPARSED_ARGUMENTS}")
	elseif (NOT _option_TARGET)
		message (FATAL_ERROR "Mandatory parameter TARGET is missing.")
	elseif (NOT _option_NAME)
		message (FATAL_ERROR "Mandatory parameter NAME is missing.")
	elseif (NOT _option_CODE AND NOT _option_SCRIPT)
		message (FATAL_ERROR "Either the keyword CODE or SCRIPT must be present.")
	endif()
	set (_cmd NAME "${_option_NAME}" COMMAND)
	_add_test_driver(_cmd "${_option_NAME}" _option_INPUT _option_INPUT_FILE)
	# run Mathematica kernel and load Wolfram library
	if (CYGWIN OR MSYS)
		get_target_property (_targetFile ${_option_TARGET} LOCATION)
		Mathematica_TO_NATIVE_PATH("${_targetFile}" _targetFileMma)
	else()
		set (_targetFileMma "\"$<TARGET_FILE:${_option_TARGET}>\"")
	endif()
	set (_installCmd
		"libPath = ${_targetFileMma}"
		"LibraryLoad[libPath]"
		"Print[LibraryLink`$LibraryError]" )
	if (_option_CODE)
		list (APPEND _installCmd ${_option_CODE})
	endif()
	if (NOT _option_SCRIPT)
		list (APPEND _installCmd "LibraryUnload[libPath]")
	endif()
	_add_kernel_launch_code(_cmd _option_SYSTEM_ID _option_KERNEL_OPTIONS)
	_add_script_or_code(_cmd _option_SCRIPT _installCmd)
	if (_option_CONFIGURATIONS)
		list (APPEND _cmd CONFIGURATIONS ${_option_CONFIGURATIONS})
	endif()
	if (Mathematica_DEBUG)
		message (STATUS "add_test: ${_cmd}")
	endif()
	add_test (${_cmd})
endfunction(Mathematica_WolframLibrary_ADD_TEST)

endif (Mathematica_KERNEL_EXECUTABLE AND Mathematica_WolframLibrary_FOUND)

if (Mathematica_WolframLibrary_FOUND)

# public function that sets dynamic library names according to LibraryLink naming conventions
function (Mathematica_WolframLibrary_SET_PROPERTIES)
	set (_haveProperties False)
	foreach (_libraryName ${ARGV})
		if ("${_libraryName}" STREQUAL "PROPERTIES")
			set (_haveProperties True)
			break()
		endif()
		set_target_properties (${_libraryName} PROPERTIES PREFIX "")
		if (WIN32 OR CYGWIN)
			set_target_properties (${_libraryName} PROPERTIES SUFFIX ".dll")
		elseif (APPLE)
			set_target_properties (${_libraryName} PROPERTIES SUFFIX ".dylib")
		elseif (UNIX)
			set_target_properties (${_libraryName} PROPERTIES SUFFIX ".so")
		endif()
		set_target_properties (${_libraryName} PROPERTIES LABELS "Mathematica")
		if (CYGWIN AND CMAKE_COMPILER_IS_GNUCC)
			# Mathematica kernel cannot load Cygwin generated libraries linked with Cygwin runtime DLL
			# a work-around is to use the -mno-cygwin flag, which is only supported by gcc 3.x, not by gcc 4.x
			if (NOT "${CMAKE_C_COMPILER_VERSION}" VERSION_LESS "3.0.0" AND "${CMAKE_C_COMPILER_VERSION}" VERSION_LESS "4.0.0")
				set_target_properties (${_libraryName} PROPERTIES COMPILE_OPTIONS "-mno-cygwin")
				set_target_properties (${_libraryName} PROPERTIES LINK_FLAGS "-mno-cygwin")
			endif()
		endif()
	endforeach()
	if (_haveProperties)
		set_target_properties (${ARGV})
	endif()
endfunction(Mathematica_WolframLibrary_SET_PROPERTIES)

# public function for creating dynamic library loadable with LibraryLink
function (Mathematica_ADD_LIBRARY _libraryName)
	add_library (${_libraryName} MODULE ${ARGN})
	Mathematica_WolframLibrary_SET_PROPERTIES(${_libraryName})
endfunction()

endif (Mathematica_WolframLibrary_FOUND)

if (Mathematica_MathLink_MPREP_EXECUTABLE)

# public function for creating source file from template file using mprep
function (Mathematica_MathLink_MPREP_TARGET _templateFile)
	get_filename_component(_templateFileName ${_templateFile} NAME)
	get_filename_component(_templateFileAbs ${_templateFile} ABSOLUTE)
	set(_options LINE_DIRECTIVES)
	set(_oneValueArgs OUTPUT CUSTOM_HEADER CUSTOM_TRAILER)
	set(_multiValueArgs "")
	cmake_parse_arguments(_option "${_options}" "${_oneValueArgs}" "${_multiValueArgs}" ${ARGN})
	if(_option_UNPARSED_ARGUMENTS)
		message (FATAL_ERROR "Unknown keywords: ${_option_UNPARSED_ARGUMENTS}")
	endif()
	if (_option_OUTPUT)
		set (_outfile ${_option_OUTPUT})
	else()
		_get_mprep_output_file("${_templateFile}" _outfile)
	endif()
	_to_native_path ("${Mathematica_MathLink_MPREP_EXECUTABLE}" _mprepExeNative)
	_to_native_path ("${_outfile}" _outfileNative)
	set (_command "${_mprepExeNative}" "-o" "${_outfileNative}")
	set (_dependencies "${Mathematica_MathLink_MPREP_EXECUTABLE}")
	if (_option_CUSTOM_HEADER)
		_to_native_path ("${_option_CUSTOM_HEADER}" _customHeaderNative)
		list (APPEND _command "-h" "${_customHeaderNative}")
		list (APPEND _dependencies "${_option_CUSTOM_HEADER}")
	endif()
	if (_option_CUSTOM_TRAILER)
		_to_native_path ("${_option_CUSTOM_TRAILER}" _customTrailerNative)
		list (APPEND _command "-t" "${_customTrailerNative}")
		list (APPEND _dependencies "${_option_CUSTOM_TRAILER}")
	endif()
	if (_option_LINE_DIRECTIVES)
		list (APPEND _command "-lines")
	else()
		list (APPEND _command "-nolines")
	endif()
	if (CYGWIN)
		# under Cygwin invoke mprep.exe with template file argument specified as
		# a relative path because it cannot handle absolute Cygwin UNIX paths
		file (RELATIVE_PATH _templateFileRel ${CMAKE_CURRENT_BINARY_DIR} ${_templateFileAbs})
		list (APPEND _command "${_templateFileRel}")
	else()
		_to_native_path ("${_templateFileAbs}" _templateFileAbsNative)
		list (APPEND _command "${_templateFileAbsNative}")
	endif()
	set (_msg "Generating MathLink source ${_outfile} from ${_templateFileName}")
	add_custom_command(
		OUTPUT ${_outfile}
		COMMAND ${_command}
		MAIN_DEPENDENCY ${_templateFileAbs}
		DEPENDS ${_dependencies}
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
		COMMENT ${_msg}
		VERBATIM)
	set_source_files_properties(${_outfile} PROPERTIES GENERATED TRUE LABELS "Mathematica")
endfunction(Mathematica_MathLink_MPREP_TARGET)

# public function for creating MathLink executable from template file and source files
function (Mathematica_MathLink_ADD_EXECUTABLE _executableName _templateFile)
	_get_mprep_output_file(${_templateFile} _outfile)
	Mathematica_MathLink_MPREP_TARGET(${_templateFile} OUTPUT ${_outfile})
	add_executable (${_executableName} WIN32 ${_outfile} ${ARGN})
	target_link_libraries(${_executableName} ${Mathematica_MathLink_LIBRARIES})
	if (Mathematica_MathLink_LINKER_FLAGS)
		set_target_properties(${_executableName} PROPERTIES LINK_FLAGS "${Mathematica_MathLink_LINKER_FLAGS}")
	endif()
	set_target_properties (${_executableName} PROPERTIES LABELS "Mathematica")
endfunction()

# public function for exporting standard mprep header and trailer code
function (Mathematica_MathLink_MPREP_EXPORT_FRAMES)
	set(_options FORCE)
	set(_oneValueArgs OUTPUT_DIRECTORY SYSTEM_ID)
	set(_multiValueArgs "")
	cmake_parse_arguments(_option "${_options}" "${_oneValueArgs}" "${_multiValueArgs}" ${ARGN})
	if (NOT _option_OUTPUT_DIRECTORY)
		set (_option_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
	endif()
	if (NOT _option_SYSTEM_ID)
		set (_option_SYSTEM_ID "${Mathematica_HOST_SYSTEM_ID}")
	endif()
	set (_headerFileName "${_option_OUTPUT_DIRECTORY}/mprep_header_${_option_SYSTEM_ID}.txt")
	set (_trailerFileName "${_option_OUTPUT_DIRECTORY}/mprep_trailer_${_option_SYSTEM_ID}.txt")
	if (NOT _option_FORCE AND
		EXISTS "${_headerFileName}" AND EXISTS "${_trailerFileName}")
		message (STATUS "Mprep header file mprep_header_${_option_SYSTEM_ID}.txt already exists")
		message (STATUS "Mprep trailer file mprep_trailer_${_option_SYSTEM_ID}.txt already exists")
		return()
	endif()
	if (WIN32)
		set (_input_file "NUL")
	else()
		set (_input_file "/dev/null")
	endif()
	execute_process(
		COMMAND "${Mathematica_MathLink_MPREP_EXECUTABLE}"
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
		INPUT_FILE "${_input_file}"
		OUTPUT_VARIABLE _mprep_frame
		OUTPUT_STRIP_TRAILING_WHITESPACE)
	# prevent CMake from interpreting ; as a list separator
	string (REPLACE ";" "\\;" _mprep_frame "${_mprep_frame}")
	string (REPLACE "\n" ";" _mprep_frame "${_mprep_frame}")
	set (_header "")
	set (_trailer "")
	foreach (_line IN LISTS _mprep_frame)
		if ("${_line}" MATCHES "MPREP_REVISION ([0-9]+)")
			set (_mprep_revision "${CMAKE_MATCH_1}")
			set (_appendToVar _header)
		elseif ("${_line}" MATCHES "/.*end header.*/")
			unset (_appendToVar)
		elseif ("${_line}" MATCHES "/.*begin trailer.*/")
			set (_appendToVar _trailer)
		elseif (DEFINED _appendToVar)
			set (${_appendToVar} "${${_appendToVar}}${_line}\n")
		endif()
	endforeach()
	if ("${_header}" MATCHES ".+")
		message (STATUS "Mprep header revision ${_mprep_revision} exported to ${_headerFileName}")
		file (WRITE "${_headerFileName}" "${_header}")
	endif()
	if ("${_trailer}" MATCHES ".+")
		message (STATUS "Mprep trailer revision ${_mprep_revision} exported to ${_trailerFileName}")
		file (WRITE "${_trailerFileName}" "${_trailer}")
	endif()
endfunction(Mathematica_MathLink_MPREP_EXPORT_FRAMES)

endif (Mathematica_MathLink_MPREP_EXECUTABLE)

if (Mathematica_WSTP_WSPREP_EXECUTABLE)

# public function for creating source file from template file using mprep
function (Mathematica_WSTP_WSPREP_TARGET _templateFile)
	get_filename_component(_templateFileName ${_templateFile} NAME)
	get_filename_component(_templateFileAbs ${_templateFile} ABSOLUTE)
	set(_options LINE_DIRECTIVES)
	set(_oneValueArgs OUTPUT CUSTOM_HEADER CUSTOM_TRAILER)
	set(_multiValueArgs "")
	cmake_parse_arguments(_option "${_options}" "${_oneValueArgs}" "${_multiValueArgs}" ${ARGN})
	if(_option_UNPARSED_ARGUMENTS)
		message (FATAL_ERROR "Unknown keywords: ${_option_UNPARSED_ARGUMENTS}")
	endif()
	if (_option_OUTPUT)
		set (_outfile ${_option_OUTPUT})
	else()
		_get_mprep_output_file("${_templateFile}" _outfile)
	endif()
	_to_native_path ("${Mathematica_WSTP_WSPREP_EXECUTABLE}" _mprepExeNative)
	_to_native_path ("${_outfile}" _outfileNative)
	set (_command "${_mprepExeNative}" "-o" "${_outfileNative}")
	set (_dependencies "${Mathematica_WSTP_WSPREP_EXECUTABLE}")
	if (_option_CUSTOM_HEADER)
		_to_native_path ("${_option_CUSTOM_HEADER}" _customHeaderNative)
		list (APPEND _command "-h" "${_customHeaderNative}")
		list (APPEND _dependencies "${_option_CUSTOM_HEADER}")
	endif()
	if (_option_CUSTOM_TRAILER)
		_to_native_path ("${_option_CUSTOM_TRAILER}" _customTrailerNative)
		list (APPEND _command "-t" "${_customTrailerNative}")
		list (APPEND _dependencies "${_option_CUSTOM_TRAILER}")
	endif()
	if (_option_LINE_DIRECTIVES)
		list (APPEND _command "-lines")
	else()
		list (APPEND _command "-nolines")
	endif()
	if (CYGWIN)
		# under Cygwin invoke mprep.exe with template file argument specified as
		# a relative path because it cannot handle absolute Cygwin UNIX paths
		file (RELATIVE_PATH _templateFileRel ${CMAKE_CURRENT_BINARY_DIR} ${_templateFileAbs})
		list (APPEND _command "${_templateFileRel}")
	else()
		_to_native_path ("${_templateFileAbs}" _templateFileAbsNative)
		list (APPEND _command "${_templateFileAbsNative}")
	endif()
	set (_msg "Generating WSTP source ${_outfile} from ${_templateFileName}")
	add_custom_command(
		OUTPUT ${_outfile}
		COMMAND ${_command}
		MAIN_DEPENDENCY ${_templateFileAbs}
		DEPENDS ${_dependencies}
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
		COMMENT ${_msg}
		VERBATIM)
	set_source_files_properties(${_outfile} PROPERTIES GENERATED TRUE LABELS "Mathematica")
endfunction(Mathematica_WSTP_WSPREP_TARGET)

# public function for creating WSTP executable from template file and source files
function (Mathematica_WSTP_ADD_EXECUTABLE _executableName _templateFile)
	_get_mprep_output_file("${_templateFile}" _outfile)
	Mathematica_WSTP_WSPREP_TARGET(${_templateFile} OUTPUT ${_outfile})
	add_executable (${_executableName} WIN32 ${_outfile} ${ARGN})
	target_link_libraries(${_executableName} ${Mathematica_WSTP_LIBRARIES})
	if (Mathematica_WSTP_LINKER_FLAGS)
		set_target_properties(${_executableName} PROPERTIES LINK_FLAGS "${Mathematica_WSTP_LINKER_FLAGS}")
	endif()
	set_target_properties (${_executableName} PROPERTIES LABELS "Mathematica")
endfunction()

# public function for exporting standard mprep header and trailer code
function (Mathematica_WSTP_WSPREP_EXPORT_FRAMES)
	set(_options FORCE)
	set(_oneValueArgs OUTPUT_DIRECTORY SYSTEM_ID)
	set(_multiValueArgs "")
	cmake_parse_arguments(_option "${_options}" "${_oneValueArgs}" "${_multiValueArgs}" ${ARGN})
	if (NOT _option_OUTPUT_DIRECTORY)
		set (_option_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
	endif()
	if (NOT _option_SYSTEM_ID)
		set (_option_SYSTEM_ID "${Mathematica_HOST_SYSTEM_ID}")
	endif()
	set (_headerFileName "${_option_OUTPUT_DIRECTORY}/wsprep_header_${_option_SYSTEM_ID}.txt")
	set (_trailerFileName "${_option_OUTPUT_DIRECTORY}/wsprep_trailer_${_option_SYSTEM_ID}.txt")
	if (NOT _option_FORCE AND
		EXISTS "${_headerFileName}" AND EXISTS "${_trailerFileName}")
		message (STATUS "wsprep header file wsprep_header_${_option_SYSTEM_ID}.txt already exists")
		message (STATUS "wsprep trailer file wsprep_trailer_${_option_SYSTEM_ID}.txt already exists")
		return()
	endif()
	if (WIN32)
		set (_input_file "NUL")
	else()
		set (_input_file "/dev/null")
	endif()
	execute_process(
		COMMAND "${Mathematica_WSTP_WSPREP_EXECUTABLE}"
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
		INPUT_FILE "${_input_file}"
		OUTPUT_VARIABLE _wsprep_frame
		OUTPUT_STRIP_TRAILING_WHITESPACE)
	# prevent CMake from interpreting ; as a list separator
	string (REPLACE ";" "\\;" _wsprep_frame "${_wsprep_frame}")
	string (REPLACE "\n" ";" _wsprep_frame "${_wsprep_frame}")
	set (_header "")
	set (_trailer "")
	foreach (_line IN LISTS _wsprep_frame)
		if ("${_line}" MATCHES "PREP_REVISION ([0-9]+)")
			set (_wsprep_revision "${CMAKE_MATCH_1}")
			set (_appendToVar _header)
		elseif ("${_line}" MATCHES "/.*end header.*/")
			unset (_appendToVar)
		elseif ("${_line}" MATCHES "/.*begin trailer.*/")
			set (_appendToVar _trailer)
		elseif (DEFINED _appendToVar)
			set (${_appendToVar} "${${_appendToVar}}${_line}\n")
		endif()
	endforeach()
	if ("${_header}" MATCHES ".+")
		message (STATUS "wsprep header revision ${_wsprep_revision} exported to ${_headerFileName}")
		file (WRITE "${_headerFileName}" "${_header}")
	endif()
	if ("${_trailer}" MATCHES ".+")
		message (STATUS "wsprep trailer revision ${_wsprep_revision} exported to ${_trailerFileName}")
		file (WRITE "${_trailerFileName}" "${_trailer}")
	endif()
endfunction(Mathematica_WSTP_WSPREP_EXPORT_FRAMES)

endif (Mathematica_WSTP_WSPREP_EXECUTABLE)

if (Mathematica_MUnit_FOUND)

# public function for resolving a TestSuite declaration in a Mathematica unit test file
function (Mathematica_MUnit_RESOLVE_SUITE _var)
	set(_options "")
	set(_oneValueArgs RELATIVE)
	set(_multiValueArgs "")
	cmake_parse_arguments(_option "${_options}" "${_oneValueArgs}" "${_multiValueArgs}" ${ARGN})
	set (${_var} "")
	foreach (_testSuiteFile IN LISTS _option_UNPARSED_ARGUMENTS)
		# parse test file names from TestSuite[{ ... }]
		file (STRINGS "${_testSuiteFile}" _testSuite NEWLINE_CONSUME)
		if ("${_testSuite}" MATCHES "TestSuite\\[")
			string (REPLACE "\n" "" _testSuite "${_testSuite}")
			string (REGEX REPLACE ".*TestSuite\\[.*{(.*)}.*\\].*" "\\1" _testSuite "${_testSuite}")
			string (REPLACE "," ";" _testSuite "${_testSuite}")
			get_filename_component(_testSuiteDir "${_testSuiteFile}" DIRECTORY)
			foreach (_testSuiteItem IN LISTS _testSuite)
				# parse quoted test file name
				string (REGEX REPLACE "[^\"]*\"(.*)\"[^\"]*" "\\1" _testSuiteItem "${_testSuiteItem}")
				_to_cmake_path("${_testSuiteDir}/${_testSuiteItem}" _testFile)
				if (_option_RELATIVE)
					file (RELATIVE_PATH _testFile "${_option_RELATIVE}" "${_testFile}")
				endif()
				list (APPEND ${_var} "${_testFile}")
			endforeach()
		else()
			# not a test suite file, return test suite file path itself
			get_filename_component(_testSuiteFile "${_testSuiteFile}" ABSOLUTE)
			if (_option_RELATIVE)
				file (RELATIVE_PATH _testSuiteFile "${_option_RELATIVE}" "${_testSuiteFile}")
			endif()
			list (APPEND ${_var} "${_testSuiteFile}")
		endif()
	endforeach()
	list (REMOVE_DUPLICATES ${_var})
	set (${_var} "${${_var}}" PARENT_SCOPE)
endfunction()

# public function for adding a CMake test that runs a Mathematica MUnit test file or notebook
function (Mathematica_MUnit_ADD_TEST)
	set(_options "")
	set(_oneValueArgs NAME LOGGERS SCRIPT INPUT INPUT_FILE TIMEOUT SYSTEM_ID)
	set(_multiValueArgs CODE CONFIGURATIONS KERNEL_OPTIONS)
	cmake_parse_arguments(_option "${_options}" "${_oneValueArgs}" "${_multiValueArgs}" ${ARGN})
	if(_option_UNPARSED_ARGUMENTS)
		message (FATAL_ERROR "Unknown keywords: ${_option_UNPARSED_ARGUMENTS}")
	elseif (NOT _option_NAME)
		message (FATAL_ERROR "Mandatory parameter NAME is missing.")
	elseif (NOT _option_SCRIPT)
		message (FATAL_ERROR "Mandatory parameter SCRIPT is missing.")
	endif()
	set (_cmd NAME "${_option_NAME}" COMMAND)
	_add_test_driver(_cmd "${_option_NAME}" _option_INPUT _option_INPUT_FILE)
	if (NOT _option_LOGGERS)
		# default to VerbosePrintLogger which prints detailed information for failed tests
		set (_option_LOGGERS "{VerbosePrintLogger[]}")
	endif()
	set (_testCmds "If[Needs[\"MUnit`\"]===$Failed,Exit[]]")
	if (_option_CODE)
		list (APPEND _testCmds ${_option_CODE})
	endif()
	if (IS_ABSOLUTE "${_option_SCRIPT}")
		_to_cmake_path("${_option_SCRIPT}" _testScript)
	else()
		_to_cmake_path("${CMAKE_CURRENT_SOURCE_DIR}/${_option_SCRIPT}" _testScript)
	endif()
	get_filename_component(_testScriptExt "${_testScript}" EXT)
	get_filename_component(_testScriptDir "${_testScript}" DIRECTORY)
	Mathematica_TO_NATIVE_STRING("${_option_NAME}" _testNameMma)
	if ("${_testScriptExt}" MATCHES "\\.(nb|cdf)$")
		# notebook test run requires Mathematica front end
		if (DEFINED Mathematica_VERSION)
			if ("${Mathematica_VERSION}" VERSION_LESS "7.0")
				# default to using undocumented function Developer`UseFrontEnd
				# available in Mathematica 5.1 and newer
				set (_useFrontEndFunc "Developer`UseFrontEnd")
			else()
				# documented function UsingFrontEnd available since Mathematica 7
				set (_useFrontEndFunc "UsingFrontEnd")
			endif()
		endif()
		Mathematica_TO_NATIVE_PATH("${_testScript}" _testScriptMma)
		string (REGEX REPLACE "\n|\t" "" _testCmd
			"${_useFrontEndFunc}[
				CompoundExpression[
					nb=NotebookOpen[${_testScriptMma},Visible->False],
					mUnitResult=TestRun[nb,TestRunTitle->${_testNameMma},Loggers:>${_option_LOGGERS}],
					NotebookClose[nb]]]")
		list (APPEND _testCmds "${_testCmd}")
	else()
		Mathematica_MUnit_RESOLVE_SUITE(_testFiles "${_testScript}")
		Mathematica_TO_NATIVE_PATH("${_testFiles}" _testFilesMma)
		list (LENGTH _testFiles _fileCount)
		if (_fileCount GREATER 1)
			if (DEFINED Mathematica_VERSION)
				if ("${Mathematica_VERSION}" VERSION_LESS "7.0")
					# default to using DirectoryName
					set (_titleExtractFunc "StringDrop[#,StringLength[DirectoryName[#]]]")
				else()
					# function FileNameTake available since Mathematica 7
					set (_titleExtractFunc "FileNameTake[#]")
				endif()
			endif()
			Mathematica_TO_NATIVE_PATH("${_testScriptDir}" _testScriptDirMma)
			string (REGEX REPLACE "\n|\t" "" _testCmd
				"mUnitResult=And@@Map[
					TestRun[#,TestRunTitle->${_titleExtractFunc},Loggers:>${_option_LOGGERS}]&,
					${_testFilesMma}]")
		else()
			string (REGEX REPLACE "\n|\t" "" _testCmd
				"mUnitResult=TestRun[
					${_testFilesMma},TestRunTitle->${_testNameMma},Loggers:>${_option_LOGGERS}]")
		endif()
		list (APPEND _testCmds "${_testCmd}")
	endif()
	# use MUnit TestRun result as exit code to signal CTest success or failure
	list (APPEND _testCmds "Exit[Boole[Not[mUnitResult]]]")
	_add_kernel_launch_code(_cmd _option_SYSTEM_ID _option_KERNEL_OPTIONS)
	_add_script_or_code(_cmd _noScript _testCmds)
	if (_option_CONFIGURATIONS)
		list (APPEND _cmd CONFIGURATIONS ${_option_CONFIGURATIONS})
	endif()
	if (Mathematica_DEBUG)
		message (STATUS "add_test: ${_cmd}")
	endif()
	add_test (${_cmd})
	set_property (TEST ${_option_NAME} PROPERTY LABELS "Mathematica")
	if (_option_TIMEOUT)
		set_tests_properties (${_option_NAME} PROPERTIES TIMEOUT ${_option_TIMEOUT})
	endif()
endfunction (Mathematica_MUnit_ADD_TEST)

endif (Mathematica_MUnit_FOUND)

if (Mathematica_KERNEL_EXECUTABLE AND Mathematica_JLink_FOUND)

# public function for adding a target which builds Mathematica documentation
function (Mathematica_ADD_DOCUMENTATION _targetName)
	# documentation build requires Apache Ant
	if (CMAKE_HOST_WIN32)
		set (_antExecutableName "ant.bat")
	else()
		set (_antExecutableName "ant")
	endif()
	find_program(Mathematica_ANT_EXECUTABLE "${_antExecutableName}" PATHS ENV ANT_HOME PATH_SUFFIXES "bin")
	if (NOT Mathematica_ANT_EXECUTABLE)
		message (WARNING "Mathematica documentation build required Apache Ant executable \"ant\" cannot be found.")
	endif()
	# find DocumentationBuild package
	Mathematica_FIND_PACKAGE(Mathematica_DocumentationBuild_PACKAGE_FILE "DocumentationBuild`"
		DOC "Mathematica DocumentationBuild package.")
	if (NOT Mathematica_DocumentationBuild_PACKAGE_FILE)
		message (STATUS "Mathematica documentation build required package \"DocumentationBuild`\" cannot be found.")
	endif()
	Mathematica_GET_PACKAGE_DIR(Mathematica_DocumentationBuild_PACKAGE_DIR
		"${Mathematica_DocumentationBuild_PACKAGE_FILE}")
	# find Transmogrify package required by DocumentationBuild package
	Mathematica_FIND_PACKAGE(Mathematica_Transmogrify_PACKAGE_FILE "Transmogrify`"
		DOC "Mathematica Transmogrify package.")
	if (NOT Mathematica_Transmogrify_PACKAGE_FILE)
		message (STATUS "Mathematica documentation build required package \"Transmogrify`\" cannot be found.")
	endif()
	mark_as_advanced(
		Mathematica_ANT_EXECUTABLE
		Mathematica_DocumentationBuild_PACKAGE_FILE
		Mathematica_Transmogrify_PACKAGE_FILE
	)
	# build command from options
	set(_options "ALL" "CHECK_TIMESTAMPS" "INCLUDE_NOTEBOOKS")
	set(_oneValueArgs DOCUMENTATION_TYPE INPUT_DIRECTORY OUTPUT_DIRECTORY APPLICATION_NAME LANGUAGE COMMENT JAVACMD)
	set(_multiValueArgs SOURCES)
	cmake_parse_arguments(_option "${_options}" "${_oneValueArgs}" "${_multiValueArgs}" ${ARGN})
	if(_option_UNPARSED_ARGUMENTS)
		message (FATAL_ERROR "Unknown keywords: ${_option_UNPARSED_ARGUMENTS}")
	endif()
	if (NOT _option_DOCUMENTATION_TYPE)
		set (_option_DOCUMENTATION_TYPE "Notebook")
	endif()
	if (NOT _option_APPLICATION_NAME)
		set (_option_APPLICATION_NAME "${PROJECT_NAME}")
	endif()
	if (NOT _option_LANGUAGE)
		set (_option_LANGUAGE "English")
	endif()
	if (NOT _option_INPUT_DIRECTORY)
		set (_option_INPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
	endif()
	if (NOT _option_JAVACMD)
		if (Mathematica_JLink_JAVA_EXECUTABLE)
			set (_option_JAVACMD "${Mathematica_JLink_JAVA_EXECUTABLE}")
		elseif (Java_JAVA_EXECUTABLE)
			set (_option_JAVACMD "${Java_JAVA_EXECUTABLE}")
		else()
			if (CMAKE_HOST_WIN32)
				set (_option_JAVACMD "java.exe")
			else()
				set (_option_JAVACMD "java")
			endif()
		endif()
	endif()
	if (NOT _option_OUTPUT_DIRECTORY)
		if (_option_DOCUMENTATION_TYPE STREQUAL "Notebook")
			set (_option_OUTPUT_DIRECTORY
				"${CMAKE_CURRENT_BINARY_DIR}/${_option_APPLICATION_NAME}/Documentation")
		else()
			set (_option_OUTPUT_DIRECTORY
				"${CMAKE_CURRENT_BINARY_DIR}/${_option_APPLICATION_NAME}-${_option_DOCUMENTATION_TYPE}")
		endif()
	endif()
	if (NOT _option_COMMENT)
		set (_option_COMMENT
			"Building ${_option_APPLICATION_NAME} Mathematica ${_option_DOCUMENTATION_TYPE} documentation")
	endif()
	# set up custom target
	set (_cmd "${_targetName}")
	if (_option_ALL)
		list (APPEND _cmd ALL)
	endif()
	if (Mathematica_ANT_EXECUTABLE AND
		Mathematica_DocumentationBuild_PACKAGE_FILE AND
		Mathematica_Transmogrify_PACKAGE_FILE)
		# set up documentation generation script if all requirements are met
		set (_templateBuildScript "${Mathematica_CMAKE_MODULE_DIR}/FindMathematicaDocumentationBuild.cmake.in")
		set (_buildScriptName "${_option_APPLICATION_NAME}Mathematica${_option_DOCUMENTATION_TYPE}DocumentationBuild.cmake")
		if (NOT EXISTS "${_templateBuildScript}")
			message (FATAL_ERROR "FindMathematica documentation build script template ${_templateBuildScript} is missing.")
		endif()
		configure_file("${_templateBuildScript}" "${_buildScriptName}" @ONLY)
		list (APPEND _cmd COMMAND "${CMAKE_COMMAND}" "-P" "${CMAKE_CURRENT_BINARY_DIR}/${_buildScriptName}")
		list (APPEND _cmd DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/${_buildScriptName}")
	else()
		# just generate empty documentation directory and print message
		list (APPEND _cmd COMMAND "${CMAKE_COMMAND}" "-E" "make_directory" "${_option_OUTPUT_DIRECTORY}")
		list (APPEND _cmd COMMAND "${CMAKE_COMMAND}" "-E" "echo" "Required Mathematica packages for documentation building are not available.")
	endif()
	list (APPEND _cmd WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
	list (APPEND _cmd COMMENT ${_option_COMMENT} VERBATIM)
	if (_option_SOURCES)
		list (APPEND _cmd SOURCES ${_option_SOURCES})
	endif()
	if (_option_INCLUDE_NOTEBOOKS)
		file (GLOB_RECURSE _docuSourceNBs "${_option_INPUT_DIRECTORY}/*.nb")
		if (_docuSourceNBs)
			if (_option_SOURCES)
				list (APPEND _cmd ${_docuSourceNBs})
			else()
				list (APPEND _cmd SOURCES ${_docuSourceNBs})
			endif()
		endif()
	endif()
	if (Mathematica_DEBUG)
		message (STATUS "add_custom_target: ${_cmd}")
	endif()
	add_custom_target(${_cmd})
endfunction (Mathematica_ADD_DOCUMENTATION)

endif (Mathematica_KERNEL_EXECUTABLE AND Mathematica_JLink_FOUND)

if (Mathematica_KERNEL_EXECUTABLE AND Mathematica_JLink_FOUND AND JAVA_FOUND)

# public function to simplify testing J/Link programs
function (Mathematica_JLink_ADD_TEST)
	set(_options "")
	set(_oneValueArgs NAME MAIN_CLASS SCRIPT TARGET INPUT INPUT_FILE SYSTEM_ID LINK_PROTOCOL)
	set(_multiValueArgs CODE CONFIGURATIONS KERNEL_OPTIONS CLASSPATH)
	cmake_parse_arguments(_option "${_options}" "${_oneValueArgs}" "${_multiValueArgs}" ${ARGN})
	if(_option_UNPARSED_ARGUMENTS)
		message (FATAL_ERROR "Unknown keywords: ${_option_UNPARSED_ARGUMENTS}")
	elseif (NOT _option_TARGET)
		message (FATAL_ERROR "Mandatory parameter TARGET is missing.")
	elseif (NOT _option_NAME)
		message (FATAL_ERROR "Mandatory parameter NAME is missing.")
	endif()
	set (_cmd NAME "${_option_NAME}" COMMAND)
	_add_test_driver(_cmd "${_option_NAME}" _option_INPUT _option_INPUT_FILE)
	if (TARGET ${_option_TARGET})
		get_target_property (_targetJarFile ${_option_TARGET} JAR_FILE)
	else()
		_to_cmake_path("${_option_TARGET}" _targetJarFile)
	endif()
	if (_option_CODE OR _option_SCRIPT)
		# run Mathematica kernel and load JAR file
		Mathematica_TO_NATIVE_PATH("${_targetJarFile}" _targetJarFileMma)
		set (_installCmd "Needs[\"JLink`\"]" "AddToClassPath[${_targetJarFileMma}]")
		if (_option_CODE)
			list (INSERT _option_CODE 0 ${_installCmd})
		else()
			set (_option_CODE ${_installCmd})
		endif()
		_add_kernel_launch_code(_cmd _option_SYSTEM_ID _option_KERNEL_OPTIONS)
		_add_script_or_code(_cmd _option_SCRIPT _option_CODE)
	else()
		# run JAR file as front-end to Mathematica kernel
		if (NOT _option_MAIN_CLASS)
			get_filename_component(_option_MAIN_CLASS ${_targetJarFile} NAME_WE)
		endif()
		_to_native_path ("${Mathematica_JLink_JAR_FILE}" _jlinkJarNative)
		_to_native_path ("${_targetJarFile}" _targetJarFileNative)
		_to_native_path_list(_classPath "${_jlinkJarNative}" "${_targetJarFileNative}" ${_option_CLASSPATH})
		if (Mathematica_JLink_JAVA_EXECUTABLE)
			list (APPEND _cmd "${Mathematica_JLink_JAVA_EXECUTABLE}")
		elseif (Java_JAVA_EXECUTABLE)
			list (APPEND _cmd "${Java_JAVA_EXECUTABLE}")
		else()
			if (CMAKE_HOST_WIN32)
				list (APPEND _cmd "java.exe")
			else()
				list (APPEND _cmd "java")
			endif()
		endif()
		if (Mathematica_JLink_RUNTIME_LIBRARY)
			get_filename_component(_jlinkLibraryDir ${Mathematica_JLink_RUNTIME_LIBRARY} DIRECTORY)
			_to_native_path ("${_jlinkLibraryDir}" _jlinkLibraryDirNative)
			list (APPEND _cmd "-Dcom.wolfram.jlink.libdir=${_jlinkLibraryDirNative}")
		endif()
		list (APPEND _cmd "-cp" "${_classPath}" "${_option_MAIN_CLASS}")
		_add_linkmode_launch_code(_cmd "-mathlink"
			_option_SYSTEM_ID _option_KERNEL_OPTIONS _option_LINK_PROTOCOL
			_option_SCRIPT _option_CODE)
	endif()
	if (_option_CONFIGURATIONS)
		list (APPEND _cmd CONFIGURATIONS ${_option_CONFIGURATIONS})
	endif()
	if (Mathematica_DEBUG)
		message (STATUS "add_test: ${_cmd}")
	endif()
	add_test (${_cmd})
endfunction(Mathematica_JLink_ADD_TEST)

endif(Mathematica_KERNEL_EXECUTABLE AND Mathematica_JLink_FOUND AND JAVA_FOUND)
