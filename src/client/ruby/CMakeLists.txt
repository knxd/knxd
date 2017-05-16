set(RESULTFILE EIBConnection.rb)

# Create result.inc
replace_defines("\\\\n\\\\1 = \\\\2" ${CMAKE_CURRENT_BINARY_DIR}/result.inc FALSE)

# Use preprocessor to generate content
add_executable(genruby gen.c)
add_custom_command(OUTPUT gen.inc
  COMMAND genruby > ${CMAKE_CURRENT_BINARY_DIR}/gen.inc
  DEPENDS genruby
  )

# Concatenate
concatenate(${RESULTFILE}
  ${CMAKE_CURRENT_SOURCE_DIR}/io.inc
  ${CMAKE_CURRENT_BINARY_DIR}/gen.inc
  ${CMAKE_CURRENT_BINARY_DIR}/result.inc
  ${CMAKE_CURRENT_SOURCE_DIR}/footer.inc
  )

add_custom_target(ruby_client ALL
  DEPENDS ${RESULTFILE}
  VERBATIM)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${RESULTFILE} DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME})
