AM_CPPFLAGS=-I$(top_srcdir)/eibd/libserver -I$(top_srcdir)/eibd/include -I$(top_srcdir)/common $(PTHSEM_CLAGS)

bin_PROGRAMS=$(EIBNET)

if HAVE_SOURCE_INFO
EIBNET=eibnetdescribe eibnetsearch
else
EIBNET=
endif
eibnetdescribe_SOURCES=eibnetdescribe.cpp
eibnetdescribe_LDADD=../libserver/libeibstack.a $(PTHSEM_LIBS)

eibnetsearch_SOURCES=eibnetsearch.cpp
eibnetsearch_LDADD=../libserver/libeibstack.a $(PTHSEM_LIBS)

