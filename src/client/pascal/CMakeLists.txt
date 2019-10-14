set(RESULTFILE EIBD.pas)

# Create result.inc
replace_defines("\\\\n\\\\nConst \\\\1 = \\\\2" ${CMAKE_CURRENT_BINARY_DIR}/result.inc TRUE)

# # Use preprocessor to generate content
# Build generator
add_executable(genpas gen.c)
add_executable(genpash genh.c)

add_custom_command(OUTPUT gen.inc
  COMMAND genpas > ${CMAKE_CURRENT_BINARY_DIR}/gen.inc
  DEPENDS genpas
  )
add_custom_command(OUTPUT genh.inc
  COMMAND genpash > ${CMAKE_CURRENT_BINARY_DIR}/genh.inc
  DEPENDS genpash
  )
# Concatenate
concatenate(${RESULTFILE}
  ${CMAKE_CURRENT_SOURCE_DIR}/header1.inc
  ${CMAKE_CURRENT_BINARY_DIR}/result.inc
  ${CMAKE_CURRENT_SOURCE_DIR}/header2.inc
  ${CMAKE_CURRENT_BINARY_DIR}/genh.inc
  ${CMAKE_CURRENT_SOURCE_DIR}/body1.inc
  ${CMAKE_CURRENT_BINARY_DIR}/gen.inc
  ${CMAKE_CURRENT_SOURCE_DIR}/body2.inc
  )

add_custom_target(pascal_client ALL
  DEPENDS ${RESULTFILE}
  VERBATIM)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${RESULTFILE} DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME})
