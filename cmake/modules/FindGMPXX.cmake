
# Find the GMPXX library and its GMP dependency.
# Simply looks for the shared libraries on Linux/OSX.
# On Windows, finds MPIR using the logic in
# ../winlibs.cmake (which works for building against
# vcpkg).

if (WIN32)
  windows_find_library(MPIR "mpir" "")
  set(GMPXX_LIBRARIES ${MPIR_LIBRARIES})
  set(GMP_LIBRARIES   ${MPIR_LIBRARIES})
  set(GMPXX_BINARIES  ${MPIR_BINARIES})
else()
  find_library(GMP_LIBRARIES   gmp   REQUIRED)
  find_library(GMPXX_LIBRARIES gmpxx REQUIRED)
endif()
