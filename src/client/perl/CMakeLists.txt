set(RESULTFILE EIBConnection.pm)

include(FindPerl)
if(PERL_FOUND)
  replace_defines("\\\\n use constant \\\\1 => \\\\2" ${CMAKE_CURRENT_BINARY_DIR}/result.inc ON)

  # Use preprocessor to generate content
  preprocess_default()

  add_custom_command(OUTPUT EIBConnection.p
    COMMAND sed -e "'s/ARRAY \\?(\\([A-Za-z][A-Za-z0-9_]*\\))/@\\1/g'" ${CMAKE_CURRENT_BINARY_DIR}/EIBConnection.post
    | ${PERL_EXECUTABLE} -MO=Deparse - > ${CMAKE_CURRENT_BINARY_DIR}/EIBConnection.p
    MAIN_DEPENDENCY EIBConnection.post
    )
  # Concatenate
  concatenate(${RESULTFILE}
    ${CMAKE_CURRENT_SOURCE_DIR}/lic.inc
    ${CMAKE_CURRENT_BINARY_DIR}/EIBConnection.p
    )

  add_custom_target(perl_client ALL
    DEPENDS ${RESULTFILE}
    VERBATIM
    )

  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${RESULTFILE} DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME})
endif()
