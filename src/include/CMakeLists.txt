add_library(includes INTERFACE)

target_include_directories(includes INTERFACE ${CMAKE_CURRENT_LIST_DIR}
                                              ${CMAKE_CURRENT_BINARY_DIR})

foreach(f config.h version.h path.h)
  configure_file("${f}.in" "${CMAKE_CURRENT_BINARY_DIR}/${f}" @ONLY)
endforeach()

install(FILES eibclient.h eibtypes.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
