CC = $(CC_FOR_BUILD)
CPPFLAGS = $(CPPFLAGS_FOR_BUILD)
CFLAGS = $(CFLAGS_FOR_BUILD)
LDFLAGS = $(LDFLAGS_FOR_BUILD)
LIBS = $(LIBS_FOR_BUILD)
EXEEXT = $(EXEEXT_FOR_BUILD)

AM_CPPFLAGS=-I$(top_srcdir)/eibd/include -I$(top_srcdir)/eibd/client -I$(top_builddir)/eibd/client

EXTRA_DIST = io.inc
CLEANFILES = gen.inc EIBConnection.py result.inc

result.inc : $(top_srcdir)/common/eibloadresult.h
	cat $<  |grep IMG_ |sed -e 's/#define \(IMG_[A-Z_0-9]\+\)  \+\([0-9]\+\)/\1 = \2/g' > $@

noinst_PROGRAMS = gen

gen_SOURCES=gen.c arg.def def.def

pkgdata_DATA = EIBConnection.py

gen.inc : gen$(EXEEXT)
	./$< > $@

EIBConnection.py : io.inc gen.inc result.inc
	cat $^ > $@
