JAVAC = @JAVAC@
JAR = @JAR@
AM_CPPFLAGS=-I$(top_srcdir)/eibd/include -I$(top_srcdir)/eibd/client -I$(top_builddir)/eibd/client

java_SRC = tuwien/auto/eibclient/Buffer.java \
 tuwien/auto/eibclient/Int8.java \
 tuwien/auto/eibclient/Int16.java \
 tuwien/auto/eibclient/Int32.java \
 tuwien/auto/eibclient/EIBAddr.java

build_SRC = tuwien/auto/eibclient/EIBConnection.java

eibclient.jar : $(java_SRC) $(build_SRC)
	rm -f tuwien/auto/eibclient/*.class
	$(JAVAC) $^
	$(JAR) -cf $@ tuwien/auto/eibclient/*.class tuwien/auto/eibclient/*.java
	rm -f tuwien/auto/eibclient/*.class

CLEANFILES = eibclient.jar EIBConnection.post result.inc $(build_SRC)
BUILT_SOURCES = eibclient.jar
EXTRA_DIST = $(java_SRC) arg.def  complete.inc  def.def  errno.inc  io.inc lic.inc EIBConnection.pre

javadir=$(datadir)/java
java_DATA = eibclient.jar

result.inc : $(top_srcdir)/common/eibloadresult.h
	cat $<  |grep IMG_ |sed -e 's/#define \(IMG_[A-Z_0-9]\+\)  \+\([0-9]\+\)/public static final int \1 = \2;/g' > $@

EIBConnection.post : result.inc

$(build_SRC) : lic.inc EIBConnection.post
	cat $^ > $@
	$(INDENT) $@ || /bin/true

@AMDEP_TRUE@-@am__include@ @am__quote@./$(DEPDIR)/EIBConnection.Po@am__quote@

.pre.post:
@am__fastdepCXX_TRUE@	if $(CXXCOMPILE) -D__cplusplus -MT $@ -MD -MP -MF "$(DEPDIR)/$*.Tpo" -E -P -x c -O0 -o $@ $<; \
@am__fastdepCXX_TRUE@	then mv -f "$(DEPDIR)/$*.Tpo" "$(DEPDIR)/$*.Po"; else rm -f "$(DEPDIR)/$*.Tpo"; exit 1; fi
@AMDEP_TRUE@@am__fastdepCXX_FALSE@	source='$<' object='$@' libtool=no @AMDEPBACKSLASH@
@AMDEP_TRUE@@am__fastdepCXX_FALSE@	DEPDIR=$(DEPDIR) $(CXXDEPMODE) $(depcomp) @AMDEPBACKSLASH@
@am__fastdepCXX_FALSE@	$(CXXCOMPILE) -D__cplusplus -E -P -x c -O0 -o $@ $<

noinst_LIBRARIES=libdummy.a
libdummy_a_SOURCES=dummy.cpp