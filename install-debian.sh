#!/bin/sh

set -ex
export LC_ALL=C.UTF-8

cd "$(dirname "$0")"

: 1 install tools, minimal variant
sudo apt-get install --no-install-recommends build-essential devscripts equivs --yes

: 2 auto-install packages required for building knxd
sudo mk-build-deps --install --tool='apt-get --no-install-recommends --yes --allow-unauthenticated' debian/control
rm -f knxd-build-deps_*.deb ../knxd_*.deb ../knxd-tools_*.deb

: 3 Build. This takes a while.
dpkg-buildpackage -b -uc

cd ..

: 4 Install knxd. Have fun.
sudo dpkg -i knxd_*.deb knxd-tools_*.deb

: 5 Clean up. Optional. Remove this if you want to rebuild soon-ish.
sudo apt-get remove --autoremove knxd-build-deps --yes
