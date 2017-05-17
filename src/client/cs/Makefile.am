pkgdata_DATA = EIBConnection.cs

AM_CPPFLAGS=-I$(top_srcdir)/src/include -I$(top_srcdir)/src/client  -I$(top_builddir)/src/client

CLEANFILES = result.inc EIBConnection.post EIBConnection.cs
BUILT_SOURCES = EIBConnection.cs
EXTRA_DIST = arg.def complete.inc def.def errno.inc help.inc io.inc lic.inc EIBConnection.pre

result.inc : $(top_srcdir)/src/common/eibloadresult.h
	sed -ne 's/#define \(IMG_[A-Z_0-9]\+\)  \+\([0-9]\+\)/public const int \1 = \2;/p' $< > $@

EIBConnection.post : result.inc

EIBConnection.cs : lic.inc EIBConnection.post
	cat $^ > $@
	$(INDENT) $@ || /bin/true

.pre.post:
@am__fastdepCXX_TRUE@	if $(CPP) $(AM_CPPFLAGS) -D__cplusplus -MT $@ -MD -MP -MF "$(DEPDIR)/$*.Tpo" -P -x c -O0 -o $@ $<; \
@am__fastdepCXX_TRUE@	then mv -f "$(DEPDIR)/$*.Tpo" "$(DEPDIR)/$*.Po"; else rm -f "$(DEPDIR)/$*.Tpo"; exit 1; fi
@AMDEP_TRUE@@am__fastdepCXX_FALSE@	source='$<' object='$@' libtool=no @AMDEPBACKSLASH@
@AMDEP_TRUE@@am__fastdepCXX_FALSE@	DEPDIR=$(DEPDIR) $(CXXDEPMODE) $(depcomp) @AMDEPBACKSLASH@
@am__fastdepCXX_FALSE@	$(CPP) $(AM_CPPFLAGS) -D__cplusplus -P -x c -O0 -o $@ $<

noinst_LIBRARIES=libdummy.a
libdummy_a_SOURCES=dummy.cpp
