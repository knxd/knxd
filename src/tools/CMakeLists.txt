add_executable(knxtool)
target_link_libraries(knxtool PRIVATE common eibclient)
target_sources(knxtool PRIVATE common.c knxtool.c)

add_executable(eibread-cgi)
target_link_libraries(eibread-cgi PRIVATE common eibclient)
target_sources(eibread-cgi PRIVATE common.c eibread-cgi.c)
set_target_properties(eibread-cgi PROPERTIES INSTALL_RPATH
                                             ${CMAKE_INSTALL_FULL_LIBDIR})

add_executable(eibwrite-cgi)
target_link_libraries(eibwrite-cgi PRIVATE common eibclient)
target_sources(eibwrite-cgi PRIVATE common.c eibwrite-cgi.c)
set_target_properties(eibwrite-cgi PROPERTIES INSTALL_RPATH
                                              ${CMAKE_INSTALL_FULL_LIBDIR})

install(
  TARGETS knxtool
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(
  TARGETS eibread-cgi eibwrite-cgi
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION "${CMAKE_INSTALL_LIBEXECDIR}/knxd")

macro(install_symlink filepath sympath)
  install(
    CODE "execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink ${filepath} ${sympath})"
  )
  install(CODE "message(\"-- Created symlink: ${sympath} -> ${filepath}\")")
endmacro(install_symlink)

set(commands
    busmonitor1
    busmonitor2
    readindividual
    progmodeon
    progmodeoff
    progmodetoggle
    progmodestatus
    maskver
    writeaddress
    vbusmonitor1
    vbusmonitor2
    mprogmodeon
    mprogmodeoff
    mprogmodetoggle
    mprogmodestatus
    mmaskver
    mpeitype
    madcread
    mread
    mwrite
    mpropread
    mpropwrite
    mpropdesc
    mpropscan
    groupread
    groupswrite
    groupwrite
    msetkey
    grouplisten
    groupresponse
    groupsresponse
    groupsocketlisten
    groupsocketread
    mpropscanpoll
    vbusmonitor1poll
    groupreadresponse
    groupcacheenable
    groupcachedisable
    groupcacheclear
    groupcacheremove
    groupcachereadsync
    groupcacheread
    mwriteplain
    mrestart
    groupsocketwrite
    groupsocketswrite
    xpropread
    xpropwrite
    groupcachelastupdates
    busmonitor3
    vbusmonitor3
    vbusmonitor1time)

file(RELATIVE_PATH symlink_target "${CMAKE_INSTALL_FULL_LIBEXECDIR}/knxd"
     "${CMAKE_INSTALL_FULL_BINDIR}/knxtool")
foreach(command ${commands})
  install_symlink("${symlink_target}"
                  "${CMAKE_INSTALL_FULL_LIBEXECDIR}/knxd/${command}")
endforeach()
