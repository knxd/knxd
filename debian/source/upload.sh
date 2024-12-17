#!/bin/sh
set -ex
D="$(git describe --tags --exact-match main)"
git push salsa main $D

git checkout debian

D="$(git describe --tags --exact-match main)"

test -s ../knxd_$D.orig.tar.gz || \
wget -O ../knxd_$D.orig.tar.gz https://salsa.debian.org/smurf/knxd/-/archive/$D/knxd-0.$D.tar.gz
dgit --dpm --ch:-sa --quilt=single build-source
debsign -S -k${KEYID:-AFD79782F3BAEC020B28A19F72CF8E5E25B4C293}
git push salsa debian

# DGIT_DRS_EMAIL_NOREPLY=smurf@debian.org dgit-repos-server debian . /usr/share/keyrings/debian-keyring.gpg,a --tag2upload https://salsa.debian.org/smurf/knxd.git "$D"
