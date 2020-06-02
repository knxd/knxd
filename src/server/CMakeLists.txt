add_executable(knxd)
target_link_libraries(knxd PRIVATE common eibstack usb)
if(HAVE_SYSTEMD)
  target_link_libraries(knxd PRIVATE PkgConfig::libsystemd)
endif()
target_compile_definitions(knxd PRIVATE "-DLIBEXECDIR=\"/usr/libexec\"")
target_sources(knxd PRIVATE knxd.cpp)

add_executable(knxd_args)
target_link_libraries(knxd_args PRIVATE server common)
target_sources(knxd_args PRIVATE knxd_args.cpp)

install(
  TARGETS knxd
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(
  TARGETS knxd_args
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_LIBEXECDIR})
