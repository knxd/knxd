set(RESULTFILE eibclient.php)

# Create result.inc
replace_defines("\\\\n const \\\\1 = \\\\2" ${CMAKE_CURRENT_BINARY_DIR}/result.inc TRUE)

# Use preprocessor to generate content
preprocess_default()

# Concatenate
concatenate(${RESULTFILE}
  ${CMAKE_CURRENT_SOURCE_DIR}/lic.inc
  ${CMAKE_CURRENT_BINARY_DIR}/EIBConnection.post
  ${CMAKE_CURRENT_SOURCE_DIR}/end.inc
  )

add_custom_target(php_client ALL
  DEPENDS ${RESULTFILE}
  VERBATIM)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${RESULTFILE} DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME})
