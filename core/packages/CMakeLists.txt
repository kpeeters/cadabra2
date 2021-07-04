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
  numeric/integrate
  numeric/evaluate
  utils/node
  utils/indices
  utils/develop
  utils/tableau
  sympy/solvers
  gauge_theory/instantons
  relativity/abstract
  relativity/schwarzschild
  )

set(COMPILED_PACKAGES
  core/_component.cc
  utils/_algorithm.cc
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

foreach(PACKAGE ${COMPILED_PACKAGES})
  get_filename_component(PINSTALL ${PACKAGE} DIRECTORY)
  get_filename_component(PNAME ${PACKAGE} NAME_WE)
  pybind11_add_module(${PNAME} SHARED ${SOURCE_PACKAGEDIR}/${PACKAGE})
  set_target_properties(${PNAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/cdb/${PINSTALL}")
  set_target_properties(${PNAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/cdb/${PINSTALL}")  
  target_link_libraries(${PNAME} PUBLIC cadabra2 ${GMPXX_LIBRARIES})
  target_include_directories(${PNAME} PUBLIC ${CADABRA_CORE_DIR})
  install(TARGETS ${PNAME} DESTINATION ${PYTHON_CDB_PACKAGE_PATH}/${PINSTALL})
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
