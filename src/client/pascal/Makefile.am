CC = $(CC_FOR_BUILD)
CPPFLAGS = $(CPPFLAGS_FOR_BUILD)
CFLAGS = $(CFLAGS_FOR_BUILD)
LDFLAGS = $(LDFLAGS_FOR_BUILD)
LIBS = $(LIBS_FOR_BUILD)
EXEEXT = $(EXEEXT_FOR_BUILD)

AM_CPPFLAGS=-I$(top_srcdir)/src/include -I$(top_srcdir)/src/client -I$(top_builddir)/src/client

EXTRA_DIST = header1.inc header2.inc body1.inc body2.inc hdef.def
CLEANFILES = genh.inc gen.inc result.inc EIBD.pas

pkgdata_DATA = EIBD.pas

result.inc : $(top_srcdir)/src/common/eibloadresult.h
	sed -ne 's/#define \(IMG_[A-Z_0-9]\+\)  \+\([0-9]\+\)/Const \1 = \2;\n/p' $< > $@

noinst_PROGRAMS = genh gen

genh_SOURCES=genh.c arg.def hdef.def
gen_SOURCES=gen.c arg.def def.def

genh.inc : genh$(EXEEXT)
	./$< > $@

gen.inc : gen$(EXEEXT)
	./$< > $@

EIBD.pas :  header1.inc result.inc header2.inc genh.inc body1.inc gen.inc body2.inc
	cat $^ > $@
