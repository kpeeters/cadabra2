cmake_minimum_required(VERSION ${CADABRA_CMAKE_VERSION}) 
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
  graphics/plot
  utils/node
  utils/indices
  utils/develop
  utils/tableau
  sympy/solvers
  sympy/calculus
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
  SET(CMAKE_SKIP_BUILD_RPATH FALSE)
  SET(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
  SET(CMAKE_INSTALL_RPATH "${PYTHON_SITE_PATH}")
  get_filename_component(PINSTALL ${PACKAGE} DIRECTORY)
  get_filename_component(PNAME ${PACKAGE} NAME_WE)
  pybind11_add_module(${PNAME} SHARED ${SOURCE_PACKAGEDIR}/${PACKAGE})
  message(STATUS "Storing algorithm C++ module as ${PINSTALL}")
  set_target_properties(${PNAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/cdb/${PINSTALL}")
  set_target_properties(${PNAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/cdb/${PINSTALL}")
  set_target_properties(${PNAME} PROPERTIES SUFFIX ".${PYTHON_MOD_SUFFIX}")

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
