AM_CPPFLAGS=-I$(top_srcdir)/src/include -I$(top_srcdir)/src/common

LDADD=common.o ../client/c/libeibclient.la
bin_PROGRAMS=busmonitor1 busmonitor2 readindividual progmodeon progmodeoff progmodetoggle progmodestatus maskver \
	writeaddress vbusmonitor1 vbusmonitor2 mprogmodeon mprogmodeoff mprogmodetoggle mprogmodestatus mmaskver \
	mpeitype madcread mread mwrite mpropread mpropwrite mpropdesc mpropscan groupread groupswrite groupwrite \
	msetkey grouplisten groupresponse groupsresponse groupsocketlisten groupsocketread mpropscanpoll \
	vbusmonitor1poll groupreadresponse groupcacheenable groupcachedisable groupcacheclear groupcacheremove \
	groupcachereadsync groupcacheread mwriteplain mrestart groupsocketwrite groupsocketswrite knxtool \
	xpropread xpropwrite groupcachelastupdates busmonitor3 vbusmonitor3

examplesdir=$(pkgdatadir)/examples
dist_examples_DATA=busmonitor1.c madcread.c mprogmodeoff.c mpropdesc.c mread.c progmodestatus.c vbusmonitor2.c \
	busmonitor2.c maskver.c mprogmodeon.c mpropread.c mwrite.c progmodetoggle.c writeaddress.c \
	common.c mmaskver.c mprogmodestatus.c mpropscan.c progmodeoff.c readindividual.c \
	common.h mpeitype.c mprogmodetoggle.c mpropwrite.c progmodeon.c vbusmonitor1.c groupread.c \
	groupwrite.c groupswrite.c msetkey.c grouplisten.c groupresponse.c groupsresponse.c \
	groupsocketlisten.c groupsocketread.c mpropscanpoll.c vbusmonitor1poll.c groupreadresponse.c \
	groupcacheenable.c groupcachedisable.c groupcacheclear.c groupcacheremove.c groupcachereadsync.c \
	groupcacheread.c mwriteplain.c mrestart.c groupsocketwrite.c groupsocketswrite.c knxtool.c \
	xpropread.c xpropwrite.c groupcachelastupdates.c busmonitor3.c vbusmonitor3.c

