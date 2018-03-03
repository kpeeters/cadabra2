file(
  GLOB 
    core_dlls
  RELATIVE 
    ${CMAKE_BINARY_DIR}/core
  "*.dll"
)

install(
  FILES
    core_dlls
  DESTINATION
    bin
)