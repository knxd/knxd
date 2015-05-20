#!/bin/sh
libtoolize --copy --force --install && \
	aclocal -I m4 --force && \
	autoheader && \
	automake --add-missing --copy --force-missing && \
	autoconf
