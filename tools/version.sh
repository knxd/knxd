#!/bin/sh
sed -ne '1s/.*(\(.*\)).*/\1/' -e '1s/-1$//' -e '1p' debian/changelog | tr -d "\n"
test -d .git || exit
git=$(git rev-parse --short HEAD)
lgit=$(git rev-parse --short $(git rev-list -1 HEAD debian/changelog) )
if test "$git" != "$lgit" ; then
	echo -n ":$git"
fi
