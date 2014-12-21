#!/bin/sh
libtoolize --copy --force --install && \
	aclocal --force && \
	autoheader && \
	automake --add-missing --copy --force-missing && \
	autoconf
