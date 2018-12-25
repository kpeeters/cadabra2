
# Find the GMPXX library and its GMP dependency.
# Simply looks for the shared libraries on Linux/OSX.
# On Windows, finds MPIR using the logic in
# ../winlibs.cmake (which works for building against
# vcpkg).

if (WIN32)
  windows_find_library(GMP_LIBRARIES mpir)
  if (GMP_LIBRARIES)
    set(GMP_FOUND TRUE)
  endif()
  windows_find_library(GMPXX_LIBRARIES mpir)
  if (GMPXX_LIBRARIES)
    set(GMPXX_FOUND TRUE)
  endif()
else()
  find_path(GMP_INCLUDE_DIRS NAMES gmp.h REQUIRED)
  find_library(GMP_LIBRARIES   gmp   REQUIRED)
  find_library(GMPXX_LIBRARIES gmpxx REQUIRED)
  message("-- Found gmp header at  ${GMP_INCLUDE_DIRS}")
  message("-- Found gmp library at ${GMP_LIBRARIES}")
  set(GMP_FOUND 1)
  set(GMPXX_FOUND 1)  
endif()

if (GMP_FOUND)
  message(STATUS "Found gmp")
else()
  message(FATAL_ERROR "Gmp not found")
endif()

if (GMPXX_FOUND)
  message(STATUS "Found gmpxx")
else()
  message(FATAL_ERROR "Gmpxx not found")
endif()
