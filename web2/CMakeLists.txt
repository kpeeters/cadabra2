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

add_custom_target(html ALL echo -n DEPENDS ${HTMLOUTDIR}/plotting.html
                                           ${HTMLOUTDIR}/beginners.html
                                           ${HTMLOUTDIR}/for_previous_users.html
                                           ${HTMLOUTDIR}/scalar_manipulations.html
                                           ${MANOUTDIR}/canonicalise.html
                 )

