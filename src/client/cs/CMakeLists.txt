set(RESULTFILE EIBConnection.cs)

# Create result.inc
replace_defines("\\\\npublic const int \\\\1 = \\\\2" ${CMAKE_CURRENT_BINARY_DIR}/result.inc TRUE)

# Use preprocessor to generate content
preprocess_default()

# Concatenate
concatenate(${RESULTFILE}
  ${CMAKE_CURRENT_SOURCE_DIR}/lic.inc
  ${CMAKE_CURRENT_BINARY_DIR}/EIBConnection.post
  )

add_custom_target(cs_client ALL
  DEPENDS ${RESULTFILE}
  VERBATIM)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${RESULTFILE} DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME})
