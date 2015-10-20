set(HTMLOUTDIR ${CMAKE_SOURCE_DIR}/web2/cadabra2/source/notebooks)
set(CNBDIR     ${CMAKE_SOURCE_DIR}/examples)
set(DC         ${CMAKE_SOURCE_DIR}/client_server/DataCell.hh ${CMAKE_SOURCE_DIR}/client_server/DataCell.cc)

add_custom_command(OUTPUT  ${HTMLOUTDIR}/beginners_segment.html 
                   DEPENDS ${CNBDIR}/beginners.cnb ${DC}
                   COMMAND cadabra2html 
                   ARGS    --segment ${CNBDIR}/beginners.cnb ${HTMLOUTDIR}/beginners_segment.html 
                   COMMENT "Creating beginners_segment.html")

add_custom_command(OUTPUT  ${HTMLOUTDIR}/plotting_segment.html 
                   DEPENDS ${CNBDIR}/plotting.cnb ${DC}
                   COMMAND cadabra2html 
                   ARGS    --segment ${CNBDIR}/plotting.cnb ${HTMLOUTDIR}/plotting_segment.html 
                   COMMENT "Creating plotting_segment.html")

add_custom_command(OUTPUT  ${HTMLOUTDIR}/for_previous_users_segment.html 
                   DEPENDS ${CNBDIR}/for_previous_users.cnb ${DC}
                   COMMAND cadabra2html 
                   ARGS    --segment ${CNBDIR}/for_previous_users.cnb ${HTMLOUTDIR}/for_previous_users_segment.html 
                   COMMENT "Creating for_previous_users.html")

add_custom_command(OUTPUT  ${HTMLOUTDIR}/scalar_manipulations_segment.html 
                   DEPENDS ${CNBDIR}/scalar_manipulations.cnb ${DC}
                   COMMAND cadabra2html 
                   ARGS    --segment ${CNBDIR}/scalar_manipulations.cnb ${HTMLOUTDIR}/scalar_manipulations_segment.html 
                   COMMENT "Creating scalar_manipulations.html")

add_custom_target(html ALL echo -n DEPENDS ${HTMLOUTDIR}/plotting_segment.html
                                           ${HTMLOUTDIR}/beginners_segment.html
                                           ${HTMLOUTDIR}/for_previous_users_segment.html
                                           ${HTMLOUTDIR}/scalar_manipulations_segment.html
                 )

