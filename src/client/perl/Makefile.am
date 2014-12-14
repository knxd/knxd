pkgdata_DATA = EIBConnection.pm

AM_CPPFLAGS=-I$(top_srcdir)/eibd/include -I$(top_srcdir)/eibd/client -I$(top_builddir)/eibd/client

result.inc : $(top_srcdir)/common/eibloadresult.h
	cat $<  |grep IMG_ |sed -e 's/#define \(IMG_[A-Z_0-9]\+\)  \+\([0-9]\+\)/ use constant \1 => \2;/g' > $@

CLEANFILES = result.inc EIBConnection.post  EIBConnection.p EIBConnection.p1 EIBConnection.pm
EXTRA_DIST = EIBConnection.pre arg.def help.inc io.inc def.def complete.inc lic.inc   
EIBConnection.post : EIBConnection.pre result.inc

@AMDEP_TRUE@-@am__include@ @am__quote@./$(DEPDIR)/EIBConnection.Po@am__quote@

EIBConnection.p1 : EIBConnection.post
	sed -e 's/ARRAY \?(\([A-Za-z][A-Za-z0-9_]*\))/@\1/g' $< > $@

EIBConnection.p : EIBConnection.p1
	perl -MO=Deparse $< > $@

EIBConnection.pm : lic.inc EIBConnection.p
	cat $^ > $@

.pre.post:
@am__fastdepCXX_TRUE@	if $(CXXCOMPILE) -D__cplusplus -MT $@ -MD -MP -MF "$(DEPDIR)/$*.Tpo" -E -P -x c -O0 -o $@ $<; \
@am__fastdepCXX_TRUE@	then mv -f "$(DEPDIR)/$*.Tpo" "$(DEPDIR)/$*.Po"; else rm -f "$(DEPDIR)/$*.Tpo"; exit 1; fi
@AMDEP_TRUE@@am__fastdepCXX_FALSE@	source='$<' object='$@' libtool=no @AMDEPBACKSLASH@
@AMDEP_TRUE@@am__fastdepCXX_FALSE@	DEPDIR=$(DEPDIR) $(CXXDEPMODE) $(depcomp) @AMDEPBACKSLASH@
@am__fastdepCXX_FALSE@	$(CXXCOMPILE) -D__cplusplus -E -P -x c -O0 -o $@ $<


noinst_LIBRARIES=libdummy.a
libdummy_a_SOURCES=dummy.cpp
