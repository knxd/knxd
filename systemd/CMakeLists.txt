install(FILES knxd.conf DESTINATION ${CMAKE_INSTALL_SYSCONFDIR})

set(RUNDIR ${CMAKE_INSTALL_PREFIX}/run)
set(sysconfdir ${CMAKE_INSTALL_FULL_SYSCONFDIR})
set(bindir ${CMAKE_INSTALL_FULL_BINDIR})
foreach(f knxd.service knxd.socket)
  configure_file("${f}.in" "${CMAKE_CURRENT_BINARY_DIR}/${f}" @ONLY)
endforeach()

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/knxd.service"
              "${CMAKE_CURRENT_BINARY_DIR}/knxd.socket"
        DESTINATION ${SYSTEMD_PREFIX}/usr/lib/systemd/system)
install(FILES sysusers.d/knxd.conf
        DESTINATION ${SYSTEMD_PREFIX}/usr/lib/sysusers.d)
