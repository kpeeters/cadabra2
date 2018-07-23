# Prints section headers
macro(print_header TEXT)
	message("")
	message("-------------------------------------------")
	message("  ${TEXT}")
	message("-------------------------------------------")
endmacro()

# Install directory permissions
macro(install_directory_permissions DIR)
	install(
		DIRECTORY DESTINATION ${DIR}
		DIRECTORY_PERMISSIONS 
		OWNER_READ 
		OWNER_WRITE 
		OWNER_EXECUTE 
		GROUP_READ 
		GROUP_EXECUTE 
		WORLD_READ 
		WORLD_EXECUTE
	)
endmacro()

# Executes rm -f on FILENAME
macro(remove_file FILENAME)
	install(CODE "execute_process(COMMAND rm -f ${FILENAME})")
endmacro()

# Inserts an install directive to copy all dlls from
# the build directory of SUBPROJECT to the Install
# bin folder
macro(install_dlls_from SUBPROJECT)
	if(CMAKE_GENERATOR MATCHES "Visual Studio.*")
		install(
			DIRECTORY "${CMAKE_BINARY_DIR}/${SUBPROJECT}/${CMAKE_BUILD_TYPE}/"
			DESTINATION bin
			FILES_MATCHING PATTERN "*.dll"
		)
	else()
		install(
			DIRECTORY "${CMAKE_BINARY_DIR}/${SUBPROJECT}/"
			DESTINATION bin
			FILES_MATCHING PATTERN "*.dll"
		)
	endif()
endmacro()
