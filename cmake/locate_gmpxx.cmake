#
# Local GMPXX
#
#   On Homebrew there seems to be constantly something wrong with
#   the pkgconfig for gmpxx. So we just add the include path by hand. 
if(APPLE)
   add_definitions("-I/usr/local/include -I/opt/local/include")
endif()
if(WIN32)
  find_path(GMP_INCLUDE_DIR gmp.h HINTS ${MPIR_ROOT}\\include\\mpir)
  find_path(GMPXX_INCLUDE_DIR gmpxx.h HINTS ${MPIR_ROOT}\\include\\mpir)
  find_library(MPIR_LIBRARY mpir HINTS ${MPIR_ROOT}\\lib)
  
  set(GMPXX_INCLUDE_DIRS
    ${GMP_INCLUDE_DIR}
	${GMPXX_INCLUDE_DIR}
  )
  set(GMP_LIB
    ${MPIR_LIBRARY}
  )
  set(GMPXX_LIB
    ${MPIR_LIBRARY}
  )
  
  include_directories(${GMPXX_INCLUDE_DIRS})
else()
  find_library(GMP_LIB gmp REQUIRED)
  find_library(GMPXX_LIB gmpxx REQUIRED)
endif()
message("--Found GMP ${GMP_LIB}")
message("--Found GMPXX ${GMPXX_LIB}")