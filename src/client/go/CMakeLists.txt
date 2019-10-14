set(RESULTFILE EIBConnection.go)

# Create result.inc
replace_defines("\\\\nconst \\\\1 = \\\\2" ${CMAKE_CURRENT_BINARY_DIR}/result.inc FALSE)

# Use preprocessor to generate content
add_executable(gengo gen.c)
add_custom_command(OUTPUT gen.inc
  COMMAND gengo > ${CMAKE_CURRENT_BINARY_DIR}/gen.inc
  DEPENDS gengo
  )

# Concatenate
concatenate(${RESULTFILE}
  ${CMAKE_CURRENT_SOURCE_DIR}/io.inc
  ${CMAKE_CURRENT_BINARY_DIR}/gen.inc
  ${CMAKE_CURRENT_BINARY_DIR}/result.inc
  )

add_custom_target(go_client ALL
  DEPENDS ${RESULTFILE}
  VERBATIM)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${RESULTFILE} DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME})
