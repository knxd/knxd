AUTOMAKE_OPTIONS=1.9
ACLOCAL_AMFLAGS = -I m4
EXTRA_DIST=SubmittingPatches .gitignore tools/version.sh

CONFIGURE_DEPENDENCIES=debian/changelog
## rebuild when the version changes

BUILDDIRS = 

SUBDIRS=. src systemd tools
DIST_SUBDIRS    = $(SUBDIRS) rpm

BUILT_SOURCES=path.h version.h
CLEANFILES=path.h version.h

path.h : Makefile
	echo "#define PKGDATADIR \"${pkgdatadir}\"" >$@
	echo "#define PKGLIBDIR \"${pkglibdir}\"" >>$@

.PHONY:	CHECK
version.h: CHECK
	echo "#define REAL_VERSION \"${shell tools/version.sh}\"" >>$@.new
	cmp -s $@.new $@ && rm $@.new || mv $@.new $@
	

# source and binary RPMs
#
.PHONY:	RPM
RPM:	all dist
	rm -f rpm/rpm-tmp.*
	make -C rpm $@


.PHONY: help HELP
help HELP:
	@echo ""
	@echo ""
	@echo "in addition to standard targets (all, install, clean, ...)"
	@echo "the following make targets may be supported (at this level):"
	@echo ""
	@echo "    make help    - print this text"
	@echo "    make RPM     - create (source and binary) RPMs"
	@echo ""
	@echo "NOTE: The RPM target may fail because it requires" 
	@echo "additional tools that may not be present on your machine."
	@echo ""

# update version number
configure: debian/changelog
config.h: configure

test: all
	sh tools/test.sh
	tools/test_inih tools/test.ini tools/bad*.ini
