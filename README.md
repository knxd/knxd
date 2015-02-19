knxd
====

This is a fork of eibd 0.0.5 (from bcusdk)
https://www.auto.tuwien.ac.at/~mkoegler/index.php/bcusdk

For a history (in German) please also see:
http://knx-user-forum.de/knx-eib-forum/38982-eibd-war-bcusdk-fork-knxd.html

## Building

### Dependencies

On Debian:

    apt-get install build-essential libtool automake
    wget https://www.auto.tuwien.ac.at/~mkoegler/pth/pthsem_2.0.8.tar.gz
    tar xzf pthsem_2.0.8.tar.gz
    cd pthsem-2.0.8
    ./configure
    make
    make install

### knxd

    ./bootstrap.sh
    ./configure
    make && make install

## Contributions

* Any contribution is *very* welcome
* Please use Github and create a pull request with your patches
* Please see SubmittingPatches to correctly Sign-Off your code and add yourself to AUTHORS (`tools/list_AUTHORS > AUTHORS`)
