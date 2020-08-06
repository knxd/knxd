#!/bin/sh

set -ex
cd "$(dirname "$0")"

: 1 install tools, minimal variant
sudo apt-get install --no-install-recommends build-essential devscripts equivs

: 2 auto-install packages required for building knxd
sudo mk-build-deps --install --tool='apt-get --no-install-recommends --yes --allow-unauthenticated' debian/control
rm -f knxd-build-deps_*.deb

: 3 Build. Takes a while.
dpkg-buildpackage -b -uc

: 4 Clean up. optional. Negate this if you want to rebuild soon-ish.
sudo apt remove --autoremove knxd-build-deps

cd ..

: 5 Install knxd. Have fun.
sudo dpkg -i knxd_*.deb knxd-tools_*.deb
