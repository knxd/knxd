#!/bin/sh
if test -f /usr/local/lib/libeibclient.so.0 ; then
	echo "*** You have old eibd libraries lying around in /usr/local/lib." >&2
	echo "*** Remove them before building or installing knxd." >&2
	exit 1
fi
case `uname` in
	Darwin*)
		LIBTOOLIZE=glibtoolize ;;
	*)
		LIBTOOLIZE=libtoolize ;;
esac

# on Debian systems we need pkg-config
if which dpkg >/dev/null && ! which pkg-config >/dev/null ; then
	echo "Package 'pkg-config' missing."
	exit 2
fi

$LIBTOOLIZE --copy --force --install && \
	aclocal -I m4 --force && \
	autoheader && \
	automake --add-missing --copy --force-missing && \
	autoconf
