
set(TOOL_LINKS busmonitor1 busmonitor2 readindividual progmodeon progmodeoff
  progmodetoggle progmodestatus maskver
  writeaddress vbusmonitor1 vbusmonitor2 mprogmodeon mprogmodeoff
  mprogmodetoggle mprogmodestatus mmaskver
  mpeitype madcread mread mwrite mpropread mpropwrite mpropdesc mpropscan
  groupread groupswrite groupwrite
  msetkey grouplisten groupresponse groupsresponse groupsocketlisten
  groupsocketread mpropscanpoll
  vbusmonitor1poll groupreadresponse groupcacheenable groupcachedisable
  groupcacheclear groupcacheremove
  groupcachereadsync groupcacheread mwriteplain mrestart groupsocketwrite
  groupsocketswrite
  xpropread xpropwrite groupcachelastupdates busmonitor3 vbusmonitor3
  vbusmonitor1time
  )

include(InstallSymlink)
foreach(_LINK ${TOOL_LINKS})
  #install_symlink(../../bin/kxntool ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBEXEC}/${_LINK})
  InstallSymlink("${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/kxntool" "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBEXECDIR}/${PROJECT_NAME}/${_LINK}")
endforeach()

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/path.h "#define PKGLIBDIR \"${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBEXECDIR}\"\n")

include_directories(${PROJECT_SOURCE_DIR}/src/common ${PROJECT_SOURCE_DIR}/src/include)

add_executable(knxtool common.c knxtool.c)
target_link_libraries(knxtool eibclient)
install(TARGETS knxtool RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_executable(eibread-cgi common.c eibread-cgi.c)
target_link_libraries(eibread-cgi eibclient)
add_executable(eibwrite-cgi common.c eibwrite-cgi.c)
target_link_libraries(eibwrite-cgi eibclient)
install(TARGETS eibread-cgi eibwrite-cgi RUNTIME DESTINATION ${CMAKE_INSTALL_LIBEXECDIR})
