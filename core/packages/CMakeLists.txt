cmake_minimum_required(VERSION 3.1) 
project(Cadabra)


#---------------------------------------------------------------------------
# Preamble.
#---------------------------------------------------------------------------

print_header("Configuring packages")

set(PYTHON_CDB_PACKAGE_PATH ${PYTHON_SITE_PATH}/cdb)
set(SOURCE_PACKAGEDIR       ${CMAKE_SOURCE_DIR}/core/packages/cdb)
set(PACKAGEDIR              ${CMAKE_BINARY_DIR}/core/packages/cdb)

#---------------------------------------------------------------------------
# Enumerate packages
#---------------------------------------------------------------------------

set(OLDPACKAGES
#  relativity/__init__
#  relativity/schwarzschild
  gauge_theory/__init__
  gauge_theory/instantons
)

set(PACKAGES
  core/component
  core/manip
  core/solve
  utils/node
  utils/develop
  sympy/solvers
  relativity/schwarzschild
  )

set(PDIRS
  relativity
  gauge_theory
)


#---------------------------------------------------------------------------
# Convert packages from cdb to python.
#---------------------------------------------------------------------------

message(STATUS "Packages built in ${PACKAGEDIR}")
file(MAKE_DIRECTORY ${PACKAGEDIR})
foreach(PDIR ${PDIRS})
   add_custom_target("${PDIR}" ALL
	             COMMAND ${CMAKE_COMMAND} -E make_directory ${PACKAGEDIR}/${PDIR}
                     COMMENT "Creating package ${PDIR}...")
endforeach()
foreach(PACKAGE ${OLDPACKAGES})
	string(REGEX REPLACE "/" "_" PACKAGETARGET ${PACKAGE})
	message(STATUS "\t${PACKAGETARGET}")					  
   add_custom_target("${PACKAGETARGET}" ALL
	                  COMMAND cadabra2python ${CMAKE_SOURCE_DIR}/core/packages/cdb/${PACKAGE}.cdb ${PACKAGEDIR}/${PACKAGE}.py
                     COMMENT "Creating ${PACKAGE}.py...")
endforeach()


#---------------------------------------------------------------------------
# Install packages
#---------------------------------------------------------------------------

foreach(PACKAGE ${OLDPACKAGES})
  get_filename_component(PINSTALL ${PACKAGE} DIRECTORY)
  # FIXME: remove old packages in the global site path
  # install(CODE     "execute_process(COMMAND rm -f ${OLD_PYTHON_SITE_PATH}/cdbcadabra2.${PYTHON_MOD_SUFFIX})")
  install(
    FILES 
      "${PACKAGEDIR}/${PACKAGE}.py" 
    DESTINATION 
      ${PYTHON_CDB_PACKAGE_PATH}/${PINSTALL}
  )
endforeach()

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
