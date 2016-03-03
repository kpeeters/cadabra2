set(HTMLOUTDIR ${CMAKE_SOURCE_DIR}/web2/cadabra2/source/notebooks)
set(TOCOUTDIR  ${CMAKE_SOURCE_DIR}/web2/cadabra2/source/)
set(MANOUTDIR  ${CMAKE_SOURCE_DIR}/web2/cadabra2/source/manual)
set(CNBDIR     ${CMAKE_SOURCE_DIR}/examples)
set(ALGODIR    ${CMAKE_SOURCE_DIR}/core/algorithms)
set(PROPDIR    ${CMAKE_SOURCE_DIR}/core/properties)
set(DC         ${CMAKE_SOURCE_DIR}/client_server/DataCell.hh ${CMAKE_SOURCE_DIR}/client_server/DataCell.cc)

add_custom_command(OUTPUT  ${HTMLOUTDIR}/beginners.html 
                   DEPENDS ${CNBDIR}/beginners.cnb ${DC}
                   COMMAND cadabra2html 
                   ARGS    --segment ${CNBDIR}/beginners.cnb ${HTMLOUTDIR}/beginners.html 
                   COMMENT "Creating beginners.html")

add_custom_command(OUTPUT  ${HTMLOUTDIR}/plotting.html 
                   DEPENDS ${CNBDIR}/plotting.cnb ${DC}
                   COMMAND cadabra2html 
                   ARGS    --segment ${CNBDIR}/plotting.cnb ${HTMLOUTDIR}/plotting.html 
                   COMMENT "Creating plotting.html")

add_custom_command(OUTPUT  ${HTMLOUTDIR}/for_previous_users.html 
                   DEPENDS ${CNBDIR}/for_previous_users.cnb ${DC}
                   COMMAND cadabra2html 
                   ARGS    --segment ${CNBDIR}/for_previous_users.cnb ${HTMLOUTDIR}/for_previous_users.html 
                   COMMENT "Creating for_previous_users.html")

add_custom_command(OUTPUT  ${HTMLOUTDIR}/schwarzschild.html 
                   DEPENDS ${CNBDIR}/schwarzschild.cnb ${DC}
                   COMMAND cadabra2html 
                   ARGS    --segment ${CNBDIR}/schwarzschild.cnb ${HTMLOUTDIR}/schwarzschild.html 
                   COMMENT "Creating schwarzschild.html")

add_custom_command(OUTPUT  ${HTMLOUTDIR}/gamma_matrix_algebra.html 
                   DEPENDS ${CNBDIR}/gamma_matrix_algebra.cnb ${DC}
                   COMMAND cadabra2html 
                   ARGS    --segment ${CNBDIR}/gamma_matrix_algebra.cnb ${HTMLOUTDIR}/gamma_matrix_algebra.html 
                   COMMENT "Creating gamma_matrix_algebra.html")

add_custom_command(OUTPUT  ${HTMLOUTDIR}/poincare_algebra.html 
                   DEPENDS ${CNBDIR}/poincare_algebra.cnb ${DC}
                   COMMAND cadabra2html 
                   ARGS    --segment ${CNBDIR}/poincare_algebra.cnb ${HTMLOUTDIR}/poincare_algebra.html 
                   COMMENT "Creating poincare_algebra.html")


# Manual pages; just add the algorithm or property name to the lists below
# and the rest will be taken care of automatically.

set(ALGOS
  collect_factors
  canonicalise
  complete
  decompose_product
  distribute
  eliminate_kronecker
  evaluate
  expand_diracbar
  factor_in
  join_gamma
)
set(PROPS
  Accent
  AntiCommuting
  Commuting
  CommutingAsProduct
  CommutingAsSum
  Coordinate
  DAntiSymmetric
  Depends
  Derivative
  Diagonal
  DiracBar
  GammaMatrix
  Indices
)

set(SCAN ${CMAKE_SOURCE_DIR}/web2/scan.py)

#add_custom_command(
#add_custom_target(algo_toc ALL echo -n DEPENDS ${TOCOUTDIR}/algo_toc.html ${SCAN})
add_custom_target(algo_toc ALL 
                  COMMAND ${SCAN} ${ALGODIR} ${ALGOS} > ${TOCOUTDIR}/algo_toc.html
                  COMMENT "Creating algorithms table-of-contents...")
add_custom_target(prop_toc ALL 
                  COMMAND ${SCAN} ${PROPDIR} ${PROPS} > ${TOCOUTDIR}/prop_toc.html
                  COMMENT "Creating properties table-of-contents...")

foreach(ALGO ${ALGOS})
   add_custom_command(OUTPUT  ${MANOUTDIR}/${ALGO}.html
	                   DEPENDS ${ALGODIR}/${ALGO}.cnb ${DC}
                      COMMAND cadabra2html
                      ARGS    --segment ${ALGODIR}/${ALGO}.cnb ${MANOUTDIR}/${ALGO}.html
                      COMMENT "Creating ${ALGO} manual page...")
   add_custom_target("${ALGO} manual" ALL echo -n DEPENDS ${MANOUTDIR}/${ALGO}.html)
#   add_dependencies(algo_toc ${ALGODIR}/${ALGO}.cnb)
endforeach()

foreach(PROP ${PROPS})
   add_custom_command(OUTPUT  ${MANOUTDIR}/${PROP}.html
	                   DEPENDS ${PROPDIR}/${PROP}.cnb ${DC}
                      COMMAND cadabra2html
                      ARGS    --segment ${PROPDIR}/${PROP}.cnb ${MANOUTDIR}/${PROP}.html
                      COMMENT "Creating ${PROP} manual page...")
   add_custom_target("${PROP} manual" ALL echo -n DEPENDS ${MANOUTDIR}/${PROP}.html)
endforeach()


add_custom_target(html ALL echo -n DEPENDS ${HTMLOUTDIR}/plotting.html
                                           ${HTMLOUTDIR}/beginners.html
                                           ${HTMLOUTDIR}/for_previous_users.html
                                           ${HTMLOUTDIR}/scalar_manipulations.html
                                           ${HTMLOUTDIR}/schwarzschild.html
                                           ${HTMLOUTDIR}/gamma_matrix_algebra.html
                                           ${HTMLOUTDIR}/poincare_algebra.html
                 )

