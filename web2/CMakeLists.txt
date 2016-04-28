set(HTMLOUTDIR ${CMAKE_SOURCE_DIR}/web2/cadabra2/source/notebooks)
set(TOCOUTDIR  ${CMAKE_SOURCE_DIR}/web2/cadabra2/source/)
set(MANOUTDIR  ${CMAKE_SOURCE_DIR}/web2/cadabra2/source/manual)
set(CNBDIR     ${CMAKE_SOURCE_DIR}/examples)
set(ALGODIR    ${CMAKE_SOURCE_DIR}/core/algorithms)
set(PROPDIR    ${CMAKE_SOURCE_DIR}/core/properties)
set(DC         ${CMAKE_SOURCE_DIR}/client_server/cadabra2html.cc ${CMAKE_SOURCE_DIR}/client_server/DataCell.hh ${CMAKE_SOURCE_DIR}/client_server/DataCell.cc)


# Sample/tutorial notebooks.

set(TUTORIALS
  beginners
  plotting
  for_previous_users
  schwarzschild
  gamma_matrix_algebra
  poincare_algebra
  string_states
  kaluza_klein
)

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
  vary
  young_project_tensor
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
  EpsilonTensor
  FilledTableau
  GammaMatrix
  ImplicitIndex
  Indices
  Integer
  InverseMetric
  KroneckerDelta
  LaTeXForm
  Metric
  NonCommuting
  PartialDerivative
  RiemannTensor
  SatisfiesBianchi
  SelfAntiCommuting
  SelfCommuting
  SelfNonCommuting
  SortOrder
  Spinor
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
   add_custom_target("${ALGO}_manual" ALL echo -n DEPENDS ${MANOUTDIR}/${ALGO}.html)
#   add_dependencies(algo_toc ${ALGODIR}/${ALGO}.cnb)
endforeach()

foreach(PROP ${PROPS})
   add_custom_command(OUTPUT  ${MANOUTDIR}/${PROP}.html
	                   DEPENDS ${PROPDIR}/${PROP}.cnb ${DC}
                      COMMAND cadabra2html
                      ARGS    --segment ${PROPDIR}/${PROP}.cnb ${MANOUTDIR}/${PROP}.html
                      COMMENT "Creating ${PROP} manual page...")
   add_custom_target("${PROP}_manual" ALL echo -n DEPENDS ${MANOUTDIR}/${PROP}.html)
endforeach()

foreach(TUT ${TUTORIALS})
  add_custom_command(OUTPUT  ${HTMLOUTDIR}/${TUT}.html
                     DEPENDS ${CNBDIR}/${TUT}.cnb ${DC}
                     COMMAND cadabra2html 
                     ARGS    --segment ${CNBDIR}/${TUT}.cnb ${HTMLOUTDIR}/${TUT}.html 
                     COMMENT "Creating ${TUT}.html")
  add_custom_target("${TUT}_html" ALL echo -n DEPENDS ${HTMLOUTDIR}/${TUT}.html)		
  add_custom_command(OUTPUT  ${HTMLOUTDIR}/${TUT}.cnb
                     DEPENDS ${CNBDIR}/${TUT}.cnb 
                     COMMAND cp
                     ARGS    ${CNBDIR}/${TUT}.cnb ${HTMLOUTDIR}/${TUT}.cnb
                     COMMENT "Creating ${TUT}.cnb")
  add_custom_target("${TUT}_cnb" ALL echo -n DEPENDS ${HTMLOUTDIR}/${TUT}.html ${HTMLOUTDIR}/${TUT}.cnb )		
endforeach()

