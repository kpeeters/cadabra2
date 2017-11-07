# TODO: EIther use FindGMPXX.cmake or delete it as it's unused right now


# Try to find the GMPXX libraries
# GMPXX_FOUND - system has GMPXX lib
# GMPXX_INCLUDE_DIR - the GMPXX include directory
# GMPXX_LIBRARIES - Libraries needed to use GMPXX
# GMPXX_RUNTIMES - DLLs for GMPXX (win32 only)


# TODO: support Windows and MacOSX

# GMPXX needs GMP

find_package( GMP QUIET )

if(GMP_FOUND)

  #if (GMPXX_INCLUDE_DIR AND GMPXX_LIBRARIES AND GMPXX_RUNTIMES)
  #  # Already in cache, be silent
  #  set(GMPXX_FIND_QUIETLY TRUE)
  #endif()

  find_path(GMPXX_INCLUDE_DIR NAMES gmpxx.h 
            PATHS ${GMP_INCLUDE_DIR_SEARCH}
            DOC "The directory containing the GMPXX include files"
           )

  find_library(GMPXX_LIBRARIES NAMES gmpxx
               PATHS ${GMP_LIBRARIES_DIR_SEARCH}
               DOC "Path to the GMPXX library"
               )
  if(WIN32)
    find_file(GMP_RUNTIME NAMES gmp.dll
  			HINTS "${GMP_LIBRARIES_DIR_SEARCH}/../bin"
  			)
    find_file(GMPXX_RUNTIME NAMES gmpxx.dll
  			HINTS "${GMP_LIBRARIES_DIR_SEARCH}/../bin"
  			)
    list(APPEND GMPXX_RUNTIMES ${GMP_RUNTIME} ${GMPXX_RUNTIME})
	message("---- GMP_RUNTIME was ${GMP_RUNTIME} and GMPXX_RUNTIME was ${GMPXX_RUNTIME} to give ${GMPXX_RUNTIMES}")
  else()
	message("---- GMPXX_RUNTIMES left unset")
    set(GMPXX_RUNTIMES "")
  endif()               
  
  find_package_handle_standard_args(GMPXX "DEFAULT_MSG" GMPXX_LIBRARIES GMPXX_INCLUDE_DIR GMPXX_RUNTIMES )

endif()
