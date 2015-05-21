AUTOMAKE_OPTIONS=1.9
ACLOCAL_AMFLAGS = -I m4
EXTRA_DIST=SubmittingPatches .gitignore

BUILDDIRS = 

SUBDIRS=. src systemd

BUILT_SOURCES=path.h
CLEANFILES=path.h

path.h : Makefile
	echo "#define PKGDATADIR \"${pkgdatadir}\"" >$@
	echo "#define PKGLIBDIR \"${pkglibdir}\"" >>$@
