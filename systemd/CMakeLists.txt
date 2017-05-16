if(SYSTEMD_FOUND)
  # systemdsystemunit_DATA=knxd.service knxd.socket
  # sysconf_DATA=knxd.conf
  # systemdsysusers_DATA=sysusers.d/knxd.conf
  # Prepare automake variables
  set(sysconfdir ${CMAKE_INSTALL_FULL_SYSCONFDIR})
  set(exec_prefix ${CMAKE_INSTALL_PREFIX})
  configure_file(knxd.service.in knxd.service)
  configure_file(knxd.socket.in knxd.socket)

  install(FILES knxd.conf DESTINATION ${CMAKE_INSTALL_SYSCONFDIR})
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/knxd.service ${CMAKE_CURRENT_BINARY_DIR}/knxd.socket
    DESTINATION $ENV{DESTDIR}${SYSTEMD_SERVICES_INSTALL_DIR})
  install(FILES sysusers.d/knxd.conf DESTINATION $ENV{DESTDIR}${SYSTEMD_SYSUSERS_INSTALL_DIR})
endif()
