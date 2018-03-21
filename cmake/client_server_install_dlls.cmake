file(
  GLOB 
    core_dlls
  RELATIVE 
    ${PROJECT_BINARY_DIR}/${CADABRA_BUILD_TYPE}/
	 "*.dll"
  )

install(
  FILES
    core_dlls
  DESTINATION
    bin
)
