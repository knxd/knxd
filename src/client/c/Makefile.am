
AUTOMAKE_OPTIONS = subdir-objects

HEADER=eibclient-int.h
NATIVE=close.c  closesync.c  complete.c  io.c  openlocal.c  openremote.c  openurl.c  pollcomplete.c  pollfd.c

FUNCS= \
  gen/getapdu.c              gen/loadimage.c         gen/mcpropertyread.c   gen/mprogmodeoff.c              gen/opentconnection.c \
  gen/getapdusrc.c           gen/mcauthorize.c       gen/mcpropertyscan.c   gen/mprogmodeon.c               gen/opentgroup.c \
  gen/getbusmonitorpacket.c  gen/mcconnect.c         gen/mcpropertywrite.c  gen/mprogmodestatus.c           gen/opentindividual.c \
  gen/getgroupsrc.c          gen/mcgetmaskversion.c  gen/mcreadadc.c        gen/mprogmodetoggle.c           gen/openttpdu.c \
  gen/groupcacheclear.c      gen/mcgetpeitype.c      gen/mcread.c           gen/mreadindividualaddresses.c  gen/openvbusmonitor.c \
  gen/groupcachedisable.c    gen/mcprogmodeoff.c     gen/mcrestart.c        gen/mwriteindividualaddress.c   gen/openvbusmonitortext.c \
  gen/groupcacheenable.c     gen/mcprogmodeon.c      gen/mcsetkey.c         gen/openbusmonitor.c            gen/reset.c \
  gen/groupcacheread.c       gen/mcprogmodestatus.c  gen/mcwrite.c          gen/openbusmonitortext.c        gen/sendapdu.c \
  gen/groupcachereadsync.c   gen/mcprogmodetoggle.c  gen/mcwriteplain.c     gen/opengroupsocket.c           gen/sendgroup.c \
  gen/groupcacheremove.c     gen/mcpropertydesc.c    gen/mgetmaskversion.c  gen/opentbroadcast.c            gen/sendtpdu.c \
  gen/gettpdu.c              gen/mcindividual.c      gen/groupcachelastupdates.c gen/openbusmonitorts.c     gen/openvbusmonitorts.c \
  gen/getbusmonitorpacketts.c

BUILT_SOURCES=$(FUNCS)
CLEANFILES=$(FUNCS)

gen/.deps/%.Plo :
gen/%.c : eibclient-int.h
	mkdir -p gen
	echo '#include "eibclient-int.h"' > $@ 
	echo '#include "def/'`basename $@|sed -e "s/\.c/.inc/g"`'"' >> $@

lib_LTLIBRARIES=libeibclient.la
AM_CPPFLAGS=-I$(top_srcdir)/src/include -I$(top_srcdir)/src/common -I$(top_srcdir)/src/client
libeibclient_la_SOURCES= $(NATIVE) $(FUNCS) $(HEADER)
libeibclient_la_LDFLAGS=-version-info 0:0:0
