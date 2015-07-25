AUTOMAKE_OPTIONS=1.9
ACLOCAL_AMFLAGS = -I m4
EXTRA_DIST=SubmittingPatches .gitignore

BUILDDIRS = 

SUBDIRS=. src systemd
DIST_SUBDIRS    = $(SUBDIRS) rpm

BUILT_SOURCES=path.h
CLEANFILES=path.h

path.h : Makefile
	echo "#define PKGDATADIR \"${pkgdatadir}\"" >$@
	echo "#define PKGLIBDIR \"${pkglibdir}\"" >>$@

# source and binary RPMs
#
# NOTE: normally RPM should depend on 'dist' and not on 'knxd-0.10.0.tar.gz'
#       However, the current build system fails sometimes (e.g. after a second
#       ./configure. Therefore, we only require one successful 'make dist'
#       until the build system is fixed.
#
#       The downside is that 'make RPM' does not rebuild the RPMs automatically
#       when sources are changed but no 'make dist' is performed.
#
.PHONY:	RPM
#RPM:	dist                       how it SHOULD be, see NOTE above
RPM:	knxd-0.10.0.tar.gz
	rm -f rpm/rpm-tmp.*
	make -C rpm $@


.PHONY: help HELP
help HELP:
	@echo ""
	@echo ""
	@echo "in addition to standard targets (all, install, clean, ...) "
	@echo "the following make targets may be supported (at this level):"
	@echo ""
	@echo "    make help    - print this text"
	@echo "    make RPM     - create (source and binary) RPMs"
	@echo ""
	@echo "NOTE: The RPM and target may fail because it requires" 
	@echo "additional tools that may not be present on your machine."
	@echo ""

