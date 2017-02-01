#!/bin/sh
sed -ne '1s/.*(\(.*\)-.*).*/\1/p' debian/changelog | tr -d "\n"
git=$(git rev-parse --short HEAD 2>/dev/null)
if [ $? = 0 ] ; then
	echo -n ":$git"
fi
