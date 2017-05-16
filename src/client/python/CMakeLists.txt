set(RESULTFILE EIBConnection.py)

# Create result.inc
replace_defines("\\\\n\\\\1 = \\\\2" ${CMAKE_CURRENT_BINARY_DIR}/result.inc FALSE)

# # Use preprocessor to generate content
# Build generator
add_executable(gen gen.c)
add_custom_command(OUTPUT gen.inc
  COMMAND gen > ${CMAKE_CURRENT_BINARY_DIR}/gen.inc
  )
# Concatenate
concatenate(${RESULTFILE}
  ${CMAKE_CURRENT_SOURCE_DIR}/io.inc
  ${CMAKE_CURRENT_BINARY_DIR}/gen.inc
  ${CMAKE_CURRENT_BINARY_DIR}/result.inc
  )

add_custom_target(python_client ALL
  DEPENDS ${RESULTFILE}
  VERBATIM)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${RESULTFILE} DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME})
