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
  calculus
  for_previous_users
  converge
  fierz
  variational_derivatives
  gamma_matrix_algebra
  poincare_algebra
  string_states
  bianchi_identities
  supergravity
  automatic_multiterm
  sphere
  schwarzschild
  frw
  kerr
  kerr_newman_cos
  exterior
  kaluza_klein
  plotting
  scalar_manipulations

  input_format
  ref_printing
  ref_patterns
  ref_numerical
  ref_ndsolve
  ref_properties
  ref_indices
  ref_derivatives
  ref_default_simplification
  ref_programming
  ref_sympy
  ref_kernel
  ref_dynamical_updates
  ref_plotting
  ref_import
  ref_exponents
  ref_spacing
  ref_implicit_versus_explicit
  ref_indexbrackets
  ref_accents
  ref_ordering
  ref_selecting
  ref_c++_library
  ref_flags_variables
  )

# The Book
set(BOOK_PARTS
   input_format
   ref_printing
   ref_patterns
   ref_numerical
   ref_ndsolve
   ref_properties
   ref_indices
   ref_derivatives
   ref_default_simplification
   ref_programming
   ref_sympy
   ref_kernel
   ref_dynamical_updates
   ref_plotting
   ref_import
   ref_exponents
   ref_spacing
   ref_implicit_versus_explicit
   ref_indexbrackets
   ref_accents
   ref_ordering
   ref_selecting
   ref_c++_library
   ref_flags_variables
)   

# 3rd party contributions

set(CONTRIBS
  einstein_equations
  structure_equations_and_bianchi
  )

# Manual pages: every notebook in core/algorithms and core/properties
# gets a page. Re-run cmake after adding a new one.

file(GLOB ALGO_CNBS RELATIVE ${ALGODIR} ${ALGODIR}/*.cnb)
string(REPLACE ".cnb" "" ALGOS "${ALGO_CNBS}")
list(SORT ALGOS)
file(GLOB PROP_CNBS RELATIVE ${PROPDIR} ${PROPDIR}/*.cnb)
string(REPLACE ".cnb" "" PROPS "${PROP_CNBS}")
list(SORT PROPS)

# Packages

set(PACKAGES
  cdb/core/manip
  cdb/core/component
  cdb/core/trace
  cdb/core/trigrat
  cdb/utils/develop
  cdb/utils/indices
  cdb/utils/node  
  cdb/utils/types    
  cdb/sympy/solvers
  cdb/sympy/calculus
  cdb/relativity/abstract
  cdb/relativity/schwarzschild
  cdb/numeric/evaluate
#  cdb/numeric/integrate
  cdb/graphics/plot
  )

# Table of contents

set(SCAN ${CMAKE_SOURCE_DIR}/web2/scan.py)

add_custom_target(algo_toc ALL 
                  COMMAND ${SCAN} algorithms ${ALGODIR} > ${TOCOUTDIR}/algo_toc.html
                  COMMENT "Creating algorithms table-of-contents...")
add_custom_target(prop_toc ALL 
                  COMMAND ${SCAN} properties ${PROPDIR} > ${TOCOUTDIR}/prop_toc.html
                  COMMENT "Creating properties table-of-contents...")
add_custom_target(package_toc ALL 
                  COMMAND ${SCAN} packages ${PACKAGEDIR} ${PACKAGES} > ${TOCOUTDIR}/packages_toc.html
                  COMMENT "Creating packages table-of-contents...")
add_custom_target(az_toc ALL 
                  COMMAND ${SCAN} az ${ALGODIR} ${PROPDIR} ${PACKAGEDIR} ${PACKAGES} > ${TOCOUTDIR}/az_toc.html
                  COMMENT "Creating alphabetical manual index...")

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

# Convert all tutorial and contrib notebooks in one go, without
# building the book: 'make notebooks'.
set(NOTEBOOK_TARGETS "")
foreach(TUT ${TUTORIALS} ${CONTRIBS})
  list(APPEND NOTEBOOK_TARGETS "${TUT}_cnb")
endforeach()
add_custom_target(notebooks)
add_dependencies(notebooks ${NOTEBOOK_TARGETS})

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

add_custom_command(
   OUTPUT   ${BOOKOUTDIR}/writing_algorithms.pdf
   DEPENDS  ${CMAKE_SOURCE_DIR}/doc/writing_algorithms.tex
   COMMAND  cp
   ARGS     ${CMAKE_SOURCE_DIR}/doc/writing_algorithms.tex
            ${CMAKE_SOURCE_DIR}/doc/autogobble.sty
            ${BOOKOUTDIR}/
   COMMAND  pdflatex
   ARGS     writing_algorithms.tex
   COMMAND  pdflatex
   ARGS     writing_algorithms.tex
   COMMAND  cp
   ARGS     writing_algorithms.pdf ${CMAKE_SOURCE_DIR}/web2/cadabra2/source
   WORKING_DIRECTORY ${BOOKOUTDIR}
   COMMENT  "Creating writing_algorithms.pdf")
add_custom_target(writing ALL echo -n DEPENDS ${BOOKOUTDIR}/writing_algorithms.pdf)

add_custom_command(
   OUTPUT   ${BOOKOUTDIR}/cadabra2_hep.pdf
   DEPENDS  ${CMAKE_SOURCE_DIR}/doc/cadabra2_hep.tex
   COMMAND  cp
   ARGS     ${CMAKE_SOURCE_DIR}/doc/cadabra2_hep.tex
            ${CMAKE_SOURCE_DIR}/doc/tableaux.sty
            ${BOOKOUTDIR}/
   COMMAND  pdflatex
   ARGS     cadabra2_hep.tex
   COMMAND  pdflatex
   ARGS     cadabra2_hep.tex
   COMMAND  cp
   ARGS     cadabra2_hep.pdf ${CMAKE_SOURCE_DIR}/web2/cadabra2/source
   WORKING_DIRECTORY ${BOOKOUTDIR}
   COMMENT  "Creating cadabra2_hep.pdf")
add_custom_target(hep_paper ALL echo -n DEPENDS ${BOOKOUTDIR}/cadabra2_hep.pdf)
