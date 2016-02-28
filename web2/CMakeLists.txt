set(HTMLOUTDIR ${CMAKE_SOURCE_DIR}/web2/cadabra2/source/notebooks)
set(MANOUTDIR  ${CMAKE_SOURCE_DIR}/web2/cadabra2/source/manual)
set(CNBDIR     ${CMAKE_SOURCE_DIR}/examples)
set(ALGODIR    ${CMAKE_SOURCE_DIR}/core/algorithms)
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

add_custom_command(OUTPUT  ${HTMLOUTDIR}/scalar_manipulations.html 
                   DEPENDS ${CNBDIR}/scalar_manipulations.cnb ${DC}
                   COMMAND cadabra2html 
                   ARGS    --segment ${CNBDIR}/scalar_manipulations.cnb ${HTMLOUTDIR}/scalar_manipulations.html 
                   COMMENT "Creating scalar_manipulations.html")


# Manual.

add_custom_command(OUTPUT  ${MANOUTDIR}/canonicalise.html 
                   DEPENDS ${ALGODIR}/canonicalise.cnb ${DC}
                   COMMAND cadabra2html 
                   ARGS    --segment ${ALGODIR}/canonicalise.cnb ${MANOUTDIR}/canonicalise.html 
                   COMMENT "Creating canonicalise.html")

add_custom_command(OUTPUT  ${MANOUTDIR}/collect_factors.html 
                   DEPENDS ${ALGODIR}/collect_factors.cnb ${DC}
                   COMMAND cadabra2html 
                   ARGS    --segment ${ALGODIR}/collect_factors.cnb ${MANOUTDIR}/collect_factors.html 
                   COMMENT "Creating collect_factors.html")

add_custom_command(OUTPUT  ${MANOUTDIR}/complete.html 
                   DEPENDS ${ALGODIR}/complete.cnb ${DC}
                   COMMAND cadabra2html 
                   ARGS    --segment ${ALGODIR}/complete.cnb ${MANOUTDIR}/complete.html 
                   COMMENT "Creating complete.html")

add_custom_command(OUTPUT  ${MANOUTDIR}/decompose_product.html 
                   DEPENDS ${ALGODIR}/decompose_product.cnb ${DC}
                   COMMAND cadabra2html 
                   ARGS    --segment ${ALGODIR}/decompose_product.cnb ${MANOUTDIR}/decompose_product.html 
                   COMMENT "Creating decompose_product.html")

add_custom_command(OUTPUT  ${MANOUTDIR}/distribute.html 
                   DEPENDS ${ALGODIR}/distribute.cnb ${DC}
                   COMMAND cadabra2html 
                   ARGS    --segment ${ALGODIR}/distribute.cnb ${MANOUTDIR}/distribute.html 
                   COMMENT "Creating distribute.html")

add_custom_command(OUTPUT  ${MANOUTDIR}/eliminate_kronecker.html 
                   DEPENDS ${ALGODIR}/eliminate_kronecker.cnb ${DC}
                   COMMAND cadabra2html 
                   ARGS    --segment ${ALGODIR}/eliminate_kronecker.cnb ${MANOUTDIR}/eliminate_kronecker.html 
                   COMMENT "Creating eliminate_kronecker.html")

add_custom_target(html ALL echo -n DEPENDS ${HTMLOUTDIR}/plotting.html
                                           ${HTMLOUTDIR}/beginners.html
                                           ${HTMLOUTDIR}/for_previous_users.html
                                           ${HTMLOUTDIR}/scalar_manipulations.html
                                           ${MANOUTDIR}/canonicalise.html
                                           ${MANOUTDIR}/collect_factors.html
                                           ${MANOUTDIR}/complete.html
                                           ${MANOUTDIR}/decompose_product.html
                                           ${MANOUTDIR}/distribute.html
                                           ${MANOUTDIR}/eliminate_kronecker.html
                 )

