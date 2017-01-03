AM_CPPFLAGS=-I$(top_srcdir)/src/libserver -I$(top_srcdir)/src/include -I$(top_srcdir)/src/common

bin_PROGRAMS=$(EIBNET)

if HAVE_SOURCE_INFO
if HAVE_EIBNETSERVER
EIBNET=eibnetdescribe eibnetsearch
else
EIBNET=
endif
else
EIBNET=
endif
eibnetdescribe_SOURCES=eibnetdescribe.cpp
eibnetdescribe_LDADD=../../libserver/libeibstack.a ../../common/libcommon.a $(EV_LIBS)

eibnetsearch_SOURCES=eibnetsearch.cpp
eibnetsearch_LDADD=../../libserver/libeibstack.a ../../common/libcommon.a $(EV_LIBS)

