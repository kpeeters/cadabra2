# On Windows, there are two options for finding required libraries:
#  - The user can provide a vcpkg toolchain file
#  - The user can provide a root directory where all the libraries
#    are installed
# If vcpkg is provided, then this takes precedence in searching for
# a library over a manually provided library location

if(WIN32)

	# Attempt to locad vcpkg toolchain. If successful, vcpkg sets the 
	# variable VCPKG_TOOLCHAIN to ON.
	if(EXISTS ${CMAKE_TOOLCHAIN_FILE})
	  debug_message("Found toolchain file...")
	  # Include the vcpkg toolchain file if provided
	  include(${CMAKE_TOOLCHAIN_FILE})
	  if(${VCPKG_TOOLCHAIN})
	    debug_message("Toolchain file is vcpkg.")
	    # Notify that vcpkg is found and print out the target triplet
	    message("Found vcpkg at ${_VCPKG_ROOT_DIR} using target triplet ${VCPKG_TARGET_TRIPLET}")
	    # Helper function which queries vcpkg if a library is installed with the current
	    # target triplet
	    macro(vcpkg_has_library libname)
		  debug_message("Checking if vcpkg has ${libname}")
		  # Create a variable POSSIBLE_MATCHES which contains a list of all installed versions of the library
		  #  - For more info on the command, type 'vcpkg help list' in the command line
		  string(TOLOWER ${libname} libnamelower)
		  execute_process(COMMAND ${_VCPKG_ROOT_DIR}/vcpkg.exe list ${libnamelower} OUTPUT_VARIABLE POSSIBLE_MATCHES)
		  # Attempt to find the target triplet in this list
		  if(NOT EXISTS ${POSSIBLE_MATCHES})
		    set(is_found NO)
		  else()
		    string(FIND ${POSSIBLE_MATCHES} ${VCPKG_TARGET_TRIPLET} is_found)
		  endif()
		  if(${is_found} EQUAL -1) # target triplet not found in POSSIBLE_MATCHES
		    debug_message("${libname} not found with vcpkg")
		    set(${libname}_VCPKG_FOUND NO)
		  else() # target triplet found in POSSIBLE_MATCHES
		    debug_message("${libname} found with vcpkg")
		    set(${libname}_VCPKG_FOUND YES)
		  endif()
	    endmacro()

	    # Helper function which sets the LIBRARY_ROOT_DIR, LIBRARY_INCLUDE_DIR, LIBRARY_LIB_DIR
	    # and LIBRARY_BIND_DIR variables for a library if it is both not already set, 
	    # and is also found with vcpkg. 
	    macro(vcpkg_find_library libname)
		  debug_message("Attempting to add ${libname} directories using vcpkg")
		  if(NOT EXISTS ${${libname}_ROOT_DIR})
		    debug_message("${libname}_ROOT_DIR not yet defined")
		    vcpkg_has_library(${libname})
		    if(${${libname}_VCPKG_FOUND})
			  set(${libname}_ROOT_DIR "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}")
			  set(${libname}_INCLUDE_DIRS "${${libname}_ROOT_DIR}/include;${${libname}_ROOT_DIR}/include/${libname}")
			  set(${libname}_LIB_DIRS "${${libname}_ROOT_DIR}/lib")
			  set(${libname}_BIN_DIRS "${${libname}_ROOT_DIR}/bin")
			  debug_message("Directories for ${libname} set")
		    else()
		      set(${libname}_ROOT_DIR ${libname}_ROOT_DIR-NOTFOUND)
			  set(${libname}_INCLUDE_DIRS ${libname}_INCLUDE_DIR-NOTFOUND)
			  set(${libname}_LIB_DIRS ${libname}_LIB_DIR-NOTFOUND)
			  set(${libname}_BIN_DIRS ${libname}_BIN_DIR-NOTFOUND)
			  debug_message("${libname} directories set to -NOTFOUND")
		    endif()
		  endif()
	    endmacro()
	  endif()
	endif()

	# Set path to a root library directory for Windows
	set(WINDOWS_LIBRARY_ROOT WINDOWS_LIBRARY_ROOT-NOTFOUND)

	# This macro makes a standard library find on Windows
	macro(windows_find_library libname libfiles dependencies)
	  # First, find dependencies
	  foreach(DEPENDENCY ${dependencies})
	    find_package(${DEPENDENCY})
	  endforeach()
	  # Find the root directory of the library
	  if(${VCPKG_TOOLCHAIN})
		vcpkg_find_library(${libname})
		debug_message("${${libname}_ROOT_DIR}")
		if(${${libname}_ROOT_DIR} STREQUAL "${libname}_ROOT_DIR-NOTFOUND")
		  # TODO: This should attempt to find the library by
		  #       looking at the WINDOWS_LIBRARY_ROOT variable
		  #       before failing with an error message
		  message(SEND_ERROR " -- Could NOT find ${libname}")
	    else()
			debug_message("Adding include directories for ${libname}: ${${libname}_INCLUDE_DIRS}")
			# KP: we do not actually need this with 'vcpkg integrate install'!
			# include_directories(${${libname}_INCLUDE_DIRS})
			debug_message("Adding link directories for ${libname}: ${${libname}_LIB_DIRS}")
			# KP: we do not actually need this with 'vcpkg integrate install'!
			link_directories(${${libname}_LIB_DIRS})
			# KP: add flag
			set(${libname}_FOUND 1)

		  set(${libname}_LIBRARIES ${libfiles})
		  foreach(DEPENDENCY ${dependencies})
		    debug_message("Adding library files ${${DEPENDENCY}_LIBRARIES} from ${DEPENDENCY} to ${libname}")
		    set(${libname}_LIBRARIES ${${libname}_LIBRARIES} ${${DEPENDENCY}_LIBRARIES})
		  endforeach()
		  debug_message("Library files for ${libname}: ${${libname}_LIBRARIES}")

		  message("Found ${libname} in ${${libname}_ROOT_DIR}")
		endif()
	  endif()
	endmacro()

endif()
