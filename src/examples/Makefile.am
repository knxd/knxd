AM_CPPFLAGS=-I$(top_srcdir)/src/include -I$(top_srcdir)/src/common

LDADD=common.o ../client/c/libeibclient.la
bin_PROGRAMS=knxtool
proglibdir=$(pkglibdir)
proglib_PROGRAMS=eibread-cgi eibwrite-cgi

links=busmonitor1 busmonitor2 readindividual progmodeon progmodeoff progmodetoggle progmodestatus maskver \
	writeaddress vbusmonitor1 vbusmonitor2 mprogmodeon mprogmodeoff mprogmodetoggle mprogmodestatus mmaskver \
	mpeitype madcread mread mwrite mpropread mpropwrite mpropdesc mpropscan groupread groupswrite groupwrite \
	msetkey grouplisten groupresponse groupsresponse groupsocketlisten groupsocketread mpropscanpoll \
	vbusmonitor1poll groupreadresponse groupcacheenable groupcachedisable groupcacheclear groupcacheremove \
	groupcachereadsync groupcacheread mwriteplain mrestart groupsocketwrite groupsocketswrite \
	xpropread xpropwrite groupcachelastupdates busmonitor3 vbusmonitor3 eibread-cgi eibwrite-cgi \
	vbusmonitor1time

install-exec-local:
	mkdir -p $(DESTDIR)/$(pkglibdir)
	for L in ${links}; do ln -sf ../../bin/knxtool $(DESTDIR)/$(pkglibdir)/$$L; done
	

examplesdir=$(pkgdatadir)/examples
dist_examples_DATA=$(addsuffix .c, ${links})

