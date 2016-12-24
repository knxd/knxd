AUTOMAKE_OPTIONS=1.9
ACLOCAL_AMFLAGS = -I m4
EXTRA_DIST=SubmittingPatches .gitignore version.sh

BUILDDIRS = 
if HAVE_PTHSEM
LIBEVDIR=libev
else
LIBEVDIR=
endif

SUBDIRS=. $(LIBEVDIR) src systemd
DIST_SUBDIRS    = $(SUBDIRS) rpm

BUILT_SOURCES=path.h
CLEANFILES=path.h

path.h : Makefile
	echo "#define PKGDATADIR \"${pkgdatadir}\"" >$@
	echo "#define PKGLIBDIR \"${pkglibdir}\"" >>$@

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

