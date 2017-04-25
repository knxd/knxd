AM_CPPFLAGS=-I$(top_srcdir)/src/include -I$(top_srcdir)/src/common

bin_PROGRAMS=knxtool

proglibdir=$(libexecdir)/knxd
proglib_PROGRAMS=eibread-cgi eibwrite-cgi

LDADD=../client/c/libeibclient.la
knxtool_SOURCES=common.h common.c knxtool.c 
eibread_cgi_SOURCES=common.h common.c eibread-cgi.c 
eibwrite_cgi_SOURCES=common.h common.c eibwrite-cgi.c 

links=busmonitor1 busmonitor2 readindividual progmodeon progmodeoff \
      progmodetoggle progmodestatus maskver \
      writeaddress vbusmonitor1 vbusmonitor2 mprogmodeon mprogmodeoff \
      mprogmodetoggle mprogmodestatus mmaskver \
      mpeitype madcread mread mwrite mpropread mpropwrite mpropdesc mpropscan \
      groupread groupswrite groupwrite \
      msetkey grouplisten groupresponse groupsresponse groupsocketlisten \
      groupsocketread mpropscanpoll \
      vbusmonitor1poll groupreadresponse groupcacheenable groupcachedisable \
      groupcacheclear groupcacheremove \
      groupcachereadsync groupcacheread mwriteplain mrestart groupsocketwrite \
      groupsocketswrite \
      xpropread xpropwrite groupcachelastupdates busmonitor3 vbusmonitor3 \
      vbusmonitor1time

install-exec-local:
	mkdir -p $(DESTDIR)/$(proglibdir)
	for L in ${links}; do ln -sf ../../bin/knxtool $(DESTDIR)/$(proglibdir)/$$L; done

## SUBDIRS=bcu eibnet
#SUBDIRS=eibnet
