# AUTOMAKE_OPTIONS = subdir-objects

# HEADER=eibclient-int.h
# NATIVE=close.c  closesync.c  complete.c  io.c  openlocal.c  openremote.c  openurl.c  pollcomplete.c  pollfd.c
set(NATIVE close.c  closesync.c  complete.c  io.c  openlocal.c  openremote.c  openurl.c  pollcomplete.c  pollfd.c)

set(FUNCS
  getapdu              loadimage         mcpropertyread   mprogmodeoff              opentconnection
  getapdusrc           mcauthorize       mcpropertyscan   mprogmodeon               opentgroup
  getbusmonitorpacket  mcconnect         mcpropertywrite  mprogmodestatus           opentindividual
  getgroupsrc          mcgetmaskversion  mcreadadc        mprogmodetoggle           openttpdu
  groupcacheclear      mcgetpeitype      mcread           mreadindividualaddresses  openvbusmonitor
  groupcachedisable    mcprogmodeoff     mcrestart        mwriteindividualaddress   openvbusmonitortext
  groupcacheenable     mcprogmodeon      mcsetkey         openbusmonitor            reset
  groupcacheread       mcprogmodestatus  mcwrite          openbusmonitortext        sendapdu
  groupcachereadsync   mcprogmodetoggle  mcwriteplain     opengroupsocket           sendgroup
  groupcacheremove     mcpropertydesc    mgetmaskversion  opentbroadcast            sendtpdu
  gettpdu              mcindividual      groupcachelastupdates openbusmonitorts     openvbusmonitorts
  getbusmonitorpacketts
  )
# Prepare command c file generator
FILE(WRITE ${CMAKE_BINARY_DIR}/genclient.cmake "
  file(WRITE \${OUTFILE} \"#include \\\"eibclient-int.h\\\"\n#include \\\"\${INFILE}\\\"\n\")
")

macro(GENERATE_FUNC _FUNC)
  set(_OUTFILE ${CMAKE_CURRENT_BINARY_DIR}/${_FUNC}.c)
  set(_DEFFILE ${CMAKE_CURRENT_SOURCE_DIR}/../def/${_FUNC}.inc)
  # file(WRITE ${_OUTFILE} "#include \"eibclient-int.h\"\n#include \"${_DEFFILE}\"\n")
  # set_source_files_properties(${_OUTFILE} PROPERTIES GENERATED TRUE OBJECT_DEPENDS ${_DEFFILE})
  add_custom_command(OUTPUT ${_OUTFILE}
    COMMAND ${CMAKE_COMMAND}
    -DINFILE=${_DEFFILE}
    -DOUTFILE=${_OUTFILE}
    -P ${CMAKE_BINARY_DIR}/genclient.cmake
    DEPENDS eibclient-int.h ${_DEFFILE}
    )
  set(FUNCS_SOURCES ${FUNCS_SOURCES} ${_OUTFILE})
endmacro()

foreach(_FUN ${FUNCS})
  generate_func(${_FUN})
endforeach()

# BUILT_SOURCES=$(FUNCS)
# CLEANFILES=$(FUNCS)

# gen/.deps/%.Plo :
# gen/%.c : eibclient-int.h
#   mkdir -p gen
#   echo '#include "eibclient-int.h"' > $@
#   echo '#include "def/'`basename $@|sed -e "s/\.c/.inc/g"`'"' >> $@

# lib_LTLIBRARIES=libeibclient.la
# AM_CPPFLAGS=-I$(top_srcdir)/src/include -I$(top_srcdir)/src/common -I$(top_srcdir)/src/client
include_directories(${PROJECT_SOURCE_DIR}/src/include ${PROJECT_SOURCE_DIR}/src/common ${PROJECT_SOURCE_DIR}/src/client)
# libeibclient_la_SOURCES= $(NATIVE) $(FUNCS) $(HEADER)
# libeibclient_la_LDFLAGS=-version-info 0:0:0

# Prepare lib creation
set(CLIENT_SOURCES ${NATIVE} ${FUNCS_SOURCES} ${HEADER})
add_library(CLIENT_OBJECTS OBJECT ${CLIENT_SOURCES})
set_property(TARGET CLIENT_OBJECTS PROPERTY POSITION_INDEPENDENT_CODE TRUE)

# Shared library
add_library(eibclient SHARED $<TARGET_OBJECTS:CLIENT_OBJECTS>)
set_property(TARGET eibclient PROPERTY VERSION "0.0.0")
set_property(TARGET eibclient PROPERTY SOVERSION 0 )
# Static library
add_library(eibclient_static STATIC $<TARGET_OBJECTS:CLIENT_OBJECTS>)
set_property(TARGET eibclient_static PROPERTY OUTPUT_NAME eibclient)
#add_library(eibclient_static STATIC ${NATIVE} ${FUNCS_SOURCES} ${HEADER})

install(TARGETS eibclient eibclient_static DESTINATION ${CMAKE_INSTALL_LIBDIR})
