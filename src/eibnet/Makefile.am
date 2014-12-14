AM_CPPFLAGS=-I$(top_srcdir)/src/libserver -I$(top_srcdir)/src/include -I$(top_srcdir)/src/common $(PTH_CFLAGS)

bin_PROGRAMS=$(EIBNET)

if HAVE_SOURCE_INFO
EIBNET=eibnetdescribe eibnetsearch
else
EIBNET=
endif
eibnetdescribe_SOURCES=eibnetdescribe.cpp
eibnetdescribe_LDADD=../libserver/libeibstack.a $(PTH_LIBS)

eibnetsearch_SOURCES=eibnetsearch.cpp
eibnetsearch_LDADD=../libserver/libeibstack.a $(PTH_LIBS)

