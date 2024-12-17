#!/bin/sh
set -ex
if ! test -v DEBSIGN_KEYID && ! test -v KEYID ; then
	echo [DEBSIGN_]KEYID not set
	exit 1
fi

D="$(git describe --tags --exact-match main)"
git push salsa main $D

git checkout debian

D="$(git describe --tags --exact-match main)"

test -s ../knxd_$D.orig.tar.gz || \
wget -O ../knxd_$D.orig.tar.gz https://salsa.debian.org/smurf/knxd/-/archive/$D/knxd-0.$D.tar.gz
dgit --dpm --ch:-sa --quilt=single build-source
debsign -S -k${DEBSIGN_KEYID:-${KEYID}}
git push salsa debian

# DGIT_DRS_EMAIL_NOREPLY=smurf@debian.org dgit-repos-server debian . /usr/share/keyrings/debian-keyring.gpg,a --tag2upload https://salsa.debian.org/smurf/knxd.git "$D"
