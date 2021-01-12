#!/bin/sh
set -ex
B="$(git describe --tags --exact-match master)"
test -n "$B"
git push salsa $B

git checkout debian

D="$(git describe --tags --exact-match debian || echo '')"
if test -z "$D" ; then
	git-debpush -u AFD79782F3BAEC020B28A19F72CF8E5E25B4C293 --upstream "$B" --remote salsa --quilt=baredebian
	D="$(git describe --tags --exact-match debian)"
fi
test -n "$D"
git push salsa $D

T=$(tempfile)
rm -f $T
mkdir $T
cd $T
trap 'cat overall.log; cd /; rm -rf $T' 0 1 2 15

DGIT_DRS_EMAIL_NOREPLY=smurf@debian.org dgit-repos-server debian . /usr/share/keyrings/debian-keyring.gpg,a --tag2upload https://salsa.debian.org/smurf/knxd.git "$D"
