CC = $(CC_FOR_BUILD)
CPPFLAGS = $(CPPFLAGS_FOR_BUILD)
CFLAGS = $(CFLAGS_FOR_BUILD)
LDFLAGS = $(LDFLAGS_FOR_BUILD)
LIBS = $(LIBS_FOR_BUILD)
EXEEXT = $(EXEEXT_FOR_BUILD)

AM_CPPFLAGS=-I$(top_srcdir)/eibd/include -I$(top_srcdir)/eibd/client -I$(top_builddir)/eibd/client

EXTRA_DIST = header1.inc header2.inc body1.inc body2.inc
CLEANFILES = genh.inc gen.inc result.inc EIBD.pas

pkgdata_DATA = EIBD.pas

result.inc : $(top_srcdir)/common/eibloadresult.h
	cat $<  |grep IMG_ |sed -e 's/#define \(IMG_[A-Z_0-9]\+\)  \+\([0-9]\+\)/Const \1 = \2;\n/g' > $@

noinst_PROGRAMS = genh gen

genh_SOURCES=genh.c arg.def hdef.def
gen_SOURCES=gen.c arg.def def.def

genh.inc : genh$(EXEEXT)
	./$< > $@

gen.inc : gen$(EXEEXT)
	./$< > $@

EIBD.pas :  header1.inc result.inc header2.inc genh.inc body1.inc gen.inc body2.inc
	cat $^ > $@
