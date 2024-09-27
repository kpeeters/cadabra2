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

# Macro just like `install`, but converting the path from a unix
# path to a windows path using `cygpath`.
macro(winstall TMP1 FILE TMP2 DEST)
  execute_process(COMMAND cygpath -m ${FILE} OUTPUT_VARIABLE WFILE)
  install(FILES ${WFILE} DESTINATION ${DEST})
endmacro()
