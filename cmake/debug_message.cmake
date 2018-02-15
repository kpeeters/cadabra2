# Function that prints a message only if CMAKE_VERBOSE_OUTPUT is set to ON
option(CMAKE_VERBOSE_OUTPUT "Print debugging info" OFF)
macro(debug_message msg)
  if(${CMAKE_VERBOSE_OUTPUT})
    message("Debug Info: ${msg}")
  endif()
endmacro()