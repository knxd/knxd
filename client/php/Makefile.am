pkgdata_DATA = eibclient.php

AM_CPPFLAGS=-I$(top_srcdir)/eibd/include -I$(top_srcdir)/eibd/client -I$(top_builddir)/eibd/client

result.inc : $(top_srcdir)/common/eibloadresult.h
	cat $<  |grep IMG_ |sed -e 's/#define \(IMG_[A-Z_0-9]\+\)  \+\([0-9]\+\)/ const \1 = \2;/g' > $@

CLEANFILES = result.inc EIBConnection.post EIBConnection.php eibclient.php
EXTRA_DIST = lic.inc end.inc arg.def  class.def  def.def complete.inc errno.inc  help.inc  io.inc EIBConnection.pre
EIBConnection.post : EIBConnection.pre result.inc

@AMDEP_TRUE@-@am__include@ @am__quote@./$(DEPDIR)/EIBConnection.Po@am__quote@

EIBConnection.php : EIBConnection.post 
	cat $< > $@
	$(INDENT) $@ || /bin/true
	sed -i -e 's/== =/===/g' $@
	sed -i -e 's/= &/=\& /g' $@

eibclient.php : lic.inc EIBConnection.php end.inc
	cat $^ > $@

.pre.post:
@am__fastdepCXX_TRUE@	if $(CXXCOMPILE) -D__cplusplus -MT $@ -MD -MP -MF "$(DEPDIR)/$*.Tpo" -E -P -x c -O0 -o $@ $<; \
@am__fastdepCXX_TRUE@	then mv -f "$(DEPDIR)/$*.Tpo" "$(DEPDIR)/$*.Po"; else rm -f "$(DEPDIR)/$*.Tpo"; exit 1; fi
@AMDEP_TRUE@@am__fastdepCXX_FALSE@	source='$<' object='$@' libtool=no @AMDEPBACKSLASH@
@AMDEP_TRUE@@am__fastdepCXX_FALSE@	DEPDIR=$(DEPDIR) $(CXXDEPMODE) $(depcomp) @AMDEPBACKSLASH@
@am__fastdepCXX_FALSE@	$(CXXCOMPILE) -D__cplusplus -E -P -x c -O0 -o $@ $<


noinst_LIBRARIES=libdummy.a
libdummy_a_SOURCES=dummy.cpp
