
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
  find_library(GMP_LIBRARIES   gmp   REQUIRED)
  find_library(GMPXX_LIBRARIES gmpxx REQUIRED)
endif()

if (GMP_FOUND)
  message(STATUS "Found gmp")
endif()

if (GMPXX_FOUND)
  message(STATUS "Found gmpxx")
endif()