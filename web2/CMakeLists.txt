print_header("Configuring documentation")

set(HTMLOUTDIR ${CMAKE_SOURCE_DIR}/web2/cadabra2/source/notebooks)
set(BOOKOUTDIR ${CMAKE_BINARY_DIR}/book)
set(TOCOUTDIR  ${CMAKE_SOURCE_DIR}/web2/cadabra2/source/)
set(MANOUTDIR  ${CMAKE_SOURCE_DIR}/web2/cadabra2/source/manual)
set(CNBDIR     ${CMAKE_SOURCE_DIR}/examples)
set(CONTRIBDIR ${CMAKE_SOURCE_DIR}/contrib)
set(ALGODIR    ${CMAKE_SOURCE_DIR}/core/algorithms)
set(PROPDIR    ${CMAKE_SOURCE_DIR}/core/properties)
set(PACKAGEDIR ${CMAKE_SOURCE_DIR}/core/packages)
set(DC         ${CMAKE_SOURCE_DIR}/client_server/cadabra2html.cc ${CMAKE_SOURCE_DIR}/core/DataCell.hh ${CMAKE_SOURCE_DIR}/core/DataCell.cc)


# Sample/tutorial notebooks.

set(TUTORIALS
  quickstart
  beginners
  tensor_monomials
  for_previous_users
  converge
  fierz
  gamma_matrix_algebra
  poincare_algebra
  string_states
  bianchi_identities
  sphere
  schwarzschild
  exterior
  kaluza_klein
  plotting
  scalar_manipulations

  input_format
  ref_printing
  ref_patterns
  ref_numerical
  ref_properties
  ref_indices
  ref_derivatives
  ref_default_simplification
  ref_programming
  ref_import
  ref_implicit_versus_explicit
  ref_selecting
  ref_c++_library
  )

# The Book
set(BOOK_PARTS
   input_format
   ref_printing
   ref_patterns
   ref_numerical
   ref_properties
   ref_indices
   ref_derivatives
   ref_default_simplification
   ref_programming
   ref_import
   ref_implicit_versus_explicit
   ref_selecting
	ref_c++_library
)   

# 3rd party contributions

set(CONTRIBS
  einstein_equations
  structure_equations_and_bianchi
  )

# Manual pages; just add the algorithm or property name to the lists below
# and the rest will be taken care of automatically.

set(ALGOS
  asym
  slot_asym
  canonicalise
  collect_factors
  collect_terms
  combine
  complete
  decompose
  decompose_product
  distribute
  drop_weight
  einsteinify
  eliminate_kronecker
  eliminate_metric
  eliminate_vielbein  
  epsilon_to_delta
  evaluate
  expand
  expand_delta
  expand_diracbar
  expand_power
  explicit_indices
  factor_in
  factor_out
  fierz
  integrate_by_parts
  join_gamma
  keep_weight
  lower_free_indices
  lr_tensor
  map_sympy
  meld
  product_rule
  raise_free_indices
  reduce_delta
  rename_dummies
  replace_match
  rewrite_indices
  simplify
  sort_product
  sort_spinors
  sort_sum
  split_gamma
  split_index
  substitute
  take_match
  untrace
  unwrap
  vary
  young_project_product
  young_project_tensor
  zoom
)
set(PROPS
  Accent
  AntiCommuting
  AntiSymmetric
  CommutingAsProduct
  CommutingAsSum
  Commuting
  Coordinate
  DAntiSymmetric
  Depends
  Derivative
  Determinant
  Diagonal
  DiracBar
  EpsilonTensor
  FilledTableau
  GammaMatrix
  ImplicitIndex
  IndexInherit
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
  Symbol
  Symmetric
  Tableau
  TableauSymmetry
  WeightInherit
)

# Packages

set(PACKAGES
  cdb/core/manip
  cdb/core/component
  cdb/core/trace
  cdb/utils/develop
  cdb/utils/indices
  cdb/utils/node  
  cdb/utils/types    
  cdb/sympy/solvers
  cdb/relativity/abstract
  cdb/relativity/schwarzschild
  cdb/numeric/evaluate
  cdb/numeric/integrate
  cdb/graphics/plot
  )

# Table of contents

set(SCAN ${CMAKE_SOURCE_DIR}/web2/scan.py)

add_custom_target(algo_toc ALL 
                  COMMAND ${SCAN} ${ALGODIR} ${ALGOS} > ${TOCOUTDIR}/algo_toc.html
                  COMMENT "Creating algorithms table-of-contents...")
add_custom_target(prop_toc ALL 
                  COMMAND ${SCAN} ${PROPDIR} ${PROPS} > ${TOCOUTDIR}/prop_toc.html
                  COMMENT "Creating properties table-of-contents...")
add_custom_target(package_toc ALL 
                  COMMAND ${SCAN} ${PACKAGEDIR} ${PACKAGES} > ${TOCOUTDIR}/packages_toc.html
                  COMMENT "Creating packages table-of-contents...")

foreach(ALGO ${ALGOS})
   add_custom_command(OUTPUT  ${MANOUTDIR}/${ALGO}.html
	                   DEPENDS ${ALGODIR}/${ALGO}.cnb ${DC}
                      COMMAND ${CMAKE_INSTALL_PREFIX}/bin/cadabra2html
                      ARGS    --segment ${ALGODIR}/${ALGO}.cnb ${MANOUTDIR}/${ALGO}.html
                      COMMENT "Creating ${ALGO} manual page...")
   add_custom_target("${ALGO}_manual" ALL echo -n DEPENDS ${MANOUTDIR}/${ALGO}.html)
endforeach()

foreach(PROP ${PROPS})
   add_custom_command(OUTPUT  ${MANOUTDIR}/${PROP}.html
	                   DEPENDS ${PROPDIR}/${PROP}.cnb ${DC}
                      COMMAND ${CMAKE_INSTALL_PREFIX}/bin/cadabra2html
                      ARGS    --segment ${PROPDIR}/${PROP}.cnb ${MANOUTDIR}/${PROP}.html
                      COMMENT "Creating ${PROP} manual page...")
   add_custom_target("${PROP}_manual" ALL echo -n DEPENDS ${MANOUTDIR}/${PROP}.html)
endforeach()

foreach(PACKAGE ${PACKAGES})
   get_filename_component(THISDESTPATH ${MANOUTDIR}/${PACKAGE}.html DIRECTORY)
   file(MAKE_DIRECTORY ${THISDESTPATH})
   add_custom_command(OUTPUT  ${MANOUTDIR}/${PACKAGE}.html
	                   DEPENDS ${PACKAGEDIR}/${PACKAGE}.cnb ${DC}
                      COMMAND ${CMAKE_INSTALL_PREFIX}/bin/cadabra2html
                      ARGS    --segment --strip-code ${PACKAGEDIR}/${PACKAGE}.cnb ${MANOUTDIR}/${PACKAGE}.html
                      COMMENT "Creating ${PACKAGE} manual page...")
	string(REPLACE "/" "_" MANGLED ${PACKAGE})
   add_custom_target("${MANGLED}_manual" ALL echo -n DEPENDS ${MANOUTDIR}/${PACKAGE}.html)
endforeach()

foreach(TUT ${TUTORIALS})
  add_custom_command(OUTPUT  ${HTMLOUTDIR}/${TUT}.html
                     DEPENDS ${CNBDIR}/${TUT}.cnb ${DC}
                     COMMAND ${CMAKE_INSTALL_PREFIX}/bin/cadabra2html 
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

foreach(TUT ${CONTRIBS})
  add_custom_command(OUTPUT  ${HTMLOUTDIR}/${TUT}.html
                     DEPENDS ${CONTRIBDIR}/${TUT}.cnb ${DC}
                     COMMAND ${CMAKE_INSTALL_PREFIX}/bin/cadabra2html 
                     ARGS    --segment ${CONTRIBDIR}/${TUT}.cnb ${HTMLOUTDIR}/${TUT}.html 
                     COMMENT "Creating ${TUT}.html")
  add_custom_target("${TUT}_html" ALL echo -n DEPENDS ${HTMLOUTDIR}/${TUT}.html)		
  add_custom_command(OUTPUT  ${HTMLOUTDIR}/${TUT}.cnb
                     DEPENDS ${CONTRIBDIR}/${TUT}.cnb 
                     COMMAND cp
                     ARGS    ${CONTRIBDIR}/${TUT}.cnb ${HTMLOUTDIR}/${TUT}.cnb
                     COMMENT "Creating ${TUT}.cnb")
  add_custom_target("${TUT}_cnb" ALL echo -n DEPENDS ${HTMLOUTDIR}/${TUT}.html ${HTMLOUTDIR}/${TUT}.cnb )		
endforeach()

file(MAKE_DIRECTORY ${BOOKOUTDIR})

# The Cadabra Book.

foreach(BOOK_PART ${BOOK_PARTS})
#   message(STATUS "Found book section ${BOOK_PART}")
   set(ALLTEX ${ALLTEX} ${BOOKOUTDIR}/${BOOK_PART}.tex)
   add_custom_command(
      OUTPUT   ${BOOKOUTDIR}/${BOOK_PART}.tex
      DEPENDS  ${CNBDIR}/${BOOK_PART}.cnb ${DC}
      COMMAND  ${CMAKE_INSTALL_PREFIX}/bin/cadabra2latex
      ARGS    --segment ${CNBDIR}/${BOOK_PART}.cnb ${BOOKOUTDIR}/${BOOK_PART}.tex
      COMMENT "Creating ${BOOK_PART}.tex")
   add_custom_target("${BOOK_PART}" ALL echo -n DEPENDS ${BOOKOUTDIR}/${BOOK_PART}.tex)
endforeach()
foreach(ALGO_PART ${ALGOS})
#   message(STATUS "Found book algorithm section ${ALGO_PART}")
   set(ALLTEX ${ALLTEX} ${BOOKOUTDIR}/${ALGO_PART}.tex)
   add_custom_command(
      OUTPUT   ${BOOKOUTDIR}/${ALGO_PART}.tex
      DEPENDS  ${ALGODIR}/${ALGO_PART}.cnb ${DC}
      COMMAND  ${CMAKE_INSTALL_PREFIX}/bin/cadabra2latex
      ARGS    --segment ${ALGODIR}/${ALGO_PART}.cnb ${BOOKOUTDIR}/${ALGO_PART}.tex
      COMMENT "Creating ${ALGO_PART}.tex")
   add_custom_target("${ALGO_PART}" ALL echo -n DEPENDS ${BOOKOUTDIR}/${ALGO_PART}.tex)
endforeach()
message(STATUS "Book will be created in ${BOOKOUTDIR}")
add_custom_command(
   OUTPUT   ${BOOKOUTDIR}/the_cadabra_book.pdf
   DEPENDS  ${CMAKE_SOURCE_DIR}/doc/the_cadabra_book.tex
            ${CMAKE_SOURCE_DIR}/doc/book_cover.pdf 
            ${CMAKE_SOURCE_DIR}/doc/the_cadabra_book.bib ${ALLTEX}   
   COMMAND  cp
   ARGS     ${CMAKE_SOURCE_DIR}/doc/the_cadabra_book.tex
            ${CMAKE_SOURCE_DIR}/doc/the_cadabra_book.bib
            ${BOOKOUTDIR}/
   COMMAND  cp
   ARGS     ${CMAKE_SOURCE_DIR}/doc/book_cover.pdf ${BOOKOUTDIR}/
   COMMAND  pdflatex
   ARGS     the_cadabra_book.tex
   COMMAND  biber
   ARGS     the_cadabra_book
   COMMAND  pdflatex
   ARGS     the_cadabra_book.tex
   COMMAND  pdflatex
   ARGS     the_cadabra_book.tex
   COMMAND  cp
   ARGS     the_cadabra_book.pdf ${CMAKE_SOURCE_DIR}/web2/cadabra2/source
   WORKING_DIRECTORY ${BOOKOUTDIR}
   COMMENT  "Creating the_cadabra_book.pdf")
add_custom_target(book ALL echo -n DEPENDS ${BOOKOUTDIR}/the_cadabra_book.pdf)


