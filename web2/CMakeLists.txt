set(HTMLOUTDIR ${CMAKE_SOURCE_DIR}/web2/cadabra2/source/notebooks)
set(CNBDIR     ${CMAKE_SOURCE_DIR}/examples)

add_custom_command(OUTPUT  ${HTMLOUTDIR}/beginners_segment.html 
                   DEPENDS ${CNBDIR}/beginners.cnb 
                   COMMAND cadabra2html 
                   ARGS    --segment ${CNBDIR}/beginners.cnb ${HTMLOUTDIR}/beginners_segment.html 
                   COMMENT "Creating beginners_segment.html")

add_custom_command(OUTPUT  ${HTMLOUTDIR}/plotting_segment.html 
                   DEPENDS ${CNBDIR}/plotting.cnb 
                   COMMAND cadabra2html 
                   ARGS    --segment ${CNBDIR}/plotting.cnb ${HTMLOUTDIR}/plotting_segment.html 
                   COMMENT "Creating plotting_segment.html")

add_custom_target(html ALL echo -n DEPENDS ${HTMLOUTDIR}/plotting_segment.html
                                           ${HTMLOUTDIR}/beginners_segment.html
                 )
