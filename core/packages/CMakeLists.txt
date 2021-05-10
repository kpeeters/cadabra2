cmake_minimum_required(VERSION 3.1) 
project(Cadabra)


#---------------------------------------------------------------------------
# Preamble.
#---------------------------------------------------------------------------

print_header("Configuring packages")

set(PYTHON_CDB_PACKAGE_PATH    ${PYTHON_SITE_PATH}/cdb)
set(OLDPYTHON_CDB_PACKAGE_PATH ${OLD_PYTHON_SITE_PATH}/cdb)
set(SOURCE_PACKAGEDIR          ${CMAKE_SOURCE_DIR}/core/packages/cdb)
set(PACKAGEDIR                 ${CMAKE_BINARY_DIR}/core/packages/cdb)

#---------------------------------------------------------------------------
# Enumerate packages
#---------------------------------------------------------------------------

set(PACKAGES
  core/component
  core/manip
  core/solve
  core/trace
  utils/node
  utils/indices
  utils/develop
  utils/tableau
  sympy/solvers
  gauge_theory/instantons
  relativity/abstract
  relativity/schwarzschild
  )


#---------------------------------------------------------------------------
# Install packages
#---------------------------------------------------------------------------

foreach(PACKAGE ${PACKAGES})
  get_filename_component(PINSTALL ${PACKAGE} DIRECTORY)
  install(
    FILES 
      "${SOURCE_PACKAGEDIR}/${PACKAGE}.cnb" 
    DESTINATION 
      ${PYTHON_CDB_PACKAGE_PATH}/${PINSTALL}
  )
endforeach()

install(
  DIRECTORY DESTINATION 
    ${PYTHON_CDB_PACKAGE_PATH} 
  DIRECTORY_PERMISSIONS 
  OWNER_READ 
  OWNER_WRITE 
  OWNER_EXECUTE
  GROUP_READ 
  GROUP_EXECUTE
  WORLD_READ 
  WORLD_EXECUTE
)
