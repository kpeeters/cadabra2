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


