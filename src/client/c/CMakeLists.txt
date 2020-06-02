add_library(eibclient SHARED)
target_link_libraries(eibclient PRIVATE includes common)
target_include_directories(eibclient PRIVATE . ..)
set_target_properties(eibclient PROPERTIES VERSION 0.0.0 SOVERSION 0)
target_sources(
  eibclient
  PRIVATE close.c
          closesync.c
          complete.c
          io.c
          openlocal.c
          openremote.c
          openurl.c
          pollcomplete.c
          pollfd.c)
set(apidefs
    getapdu
    getapdusrc
    getbusmonitorpacket
    getbusmonitorpacketts
    getgroupsrc
    gettpdu
    groupcacheclear
    groupcachedisable
    groupcacheenable
    groupcachelastupdates
    groupcacheread
    groupcachereadsync
    groupcacheremove
    loadimage
    mcauthorize
    mcconnect
    mcgetmaskversion
    mcgetpeitype
    mcindividual
    mcprogmodeoff
    mcprogmodeon
    mcprogmodestatus
    mcprogmodetoggle
    mcpropertydesc
    mcpropertyread
    mcpropertyscan
    mcpropertywrite
    mcreadadc
    mcread
    mcrestart
    mcsetkey
    mcwrite
    mcwriteplain
    mgetmaskversion
    mprogmodeoff
    mprogmodeon
    mprogmodestatus
    mprogmodetoggle
    mreadindividualaddresses
    mwriteindividualaddress
    openbusmonitor
    openbusmonitortext
    openbusmonitorts
    opengroupsocket
    opentbroadcast
    opentconnection
    opentgroup
    opentindividual
    openttpdu
    openvbusmonitor
    openvbusmonitortext
    openvbusmonitorts
    reset
    sendapdu
    sendgroup
    sendtpdu)
foreach(apidef ${apidefs})
  set(include_file "def/${apidef}.inc")
  set(source_file "${CMAKE_CURRENT_BINARY_DIR}/${apidef}.c")
  file(WRITE ${source_file}
       "#include \"eibclient-int.h\"\n#include \"${include_file}\"\n")
  target_sources(eibclient PRIVATE ${source_file})
endforeach()


install(
  TARGETS eibclient
  EXPORT EibClientConfig
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(EXPORT EibClientConfig DESTINATION share/knxd/cmake)
export(TARGETS eibclient FILE EibClientConfig.cmake)

