AM_CPPFLAGS=-I$(top_srcdir)/src/include -I$(top_srcdir)/src/common

LDADD=common.o ../client/c/libeibclient.la
bin_PROGRAMS=

if HAVE_PTHSEM
SUBDIRS=bcu eibnet
else
SUBDIRS=
endif
