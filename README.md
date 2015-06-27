knxd [![Build Status](https://travis-ci.org/knxd/knxd.svg)](https://travis-ci.org/knxd/knxd)
====

This is a fork of eibd 0.0.5 (from bcusdk)
https://www.auto.tuwien.ac.at/~mkoegler/index.php/bcusdk

For a (german only) history and discussion why knxd emerged please also see: [eibd(war bcusdk) Fork -> knxd](http://knx-user-forum.de/forum/öffentlicher-bereich/knx-eib-forum/39972-eibd-war-bcusdk-fork-knxd)

## New Features

* 0.10
** Support for more than one KNX interface

## Building

### Getting the source code

On Debian:

    apt-get install git-core
    git clone https://github.com/knxd/knxd.git

### Dependencies

On Debian:

    apt-get install build-essential libtool automake pkg-config cdbs
    wget https://www.auto.tuwien.ac.at/~mkoegler/pth/pthsem_2.0.8.tar.gz
    tar xzf pthsem_2.0.8.tar.gz
    cd pthsem-2.0.8
    dpkg-buildpackage -b
    cd ..
    sudo dpkg -i libpthsem*.deb

### knxd

    cd knxd
    dpkg-buildpackage -b
    cd ..
    sudo dpkg -i knxd_*.deb knxd-tools_*.deb

## Contributions

* Any contribution is *very* welcome
* Please use Github and create a pull request with your patches
* Please see SubmittingPatches to correctly Sign-Off your code and add yourself to AUTHORS (`tools/list_AUTHORS > AUTHORS`)
* Adhere to our [coding conventions](https://github.com/knxd/knxd/wiki/CodingConventions).
