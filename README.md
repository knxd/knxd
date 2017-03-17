knxd [![Build Status](https://travis-ci.org/knxd/knxd.svg)](https://travis-ci.org/knxd/knxd)
====

KNX is a very common building automation protocol which runs on dedicated 9600-baud wire as well as IP multicast.
``knxd`` is an advanced router/gateway which runs on any Linux computer; it can talk to all known KNX interfaces.

This code is a fork of eibd 0.0.5 (from bcusdk)
https://www.auto.tuwien.ac.at/~mkoegler/index.php/bcusdk

For a (german only) history and discussion why knxd emerged please also see: [eibd(war bcusdk) Fork -> knxd](http://knx-user-forum.de/forum/öffentlicher-bereich/knx-eib-forum/39972-eibd-war-bcusdk-fork-knxd)

# This version is outdated

v0.10 is not maintained any more.

Check [the Wiki page](https://github.com/knxd/knxd/wiki) for the version(s) to use.

## New Features

* 0.10 (this branch)

  * Support for more than one KNX interface

  * Lots of bug fixes

  * ETS5 compatibility

## Building

On Debian:

    # Do not use "sudo" unless told to do so.
    # If "dpkg-buildpackage" complains about missing packages
    # ("Unmet build dependencies"): install them
    # (apt-get install …) and try that step again.
    # If it wants "x | y", try just x; install y if that doesn't work.
    # Also, if it complains about conflicting packages, remove them (duh).

    # first, install build tools and get the source code
    sudo apt-get install git-core build-essential
    git clone https://github.com/knxd/knxd.git

    # knxd requires libpthsem which unfortunately isn't part of Debian
    wget https://www.auto.tuwien.ac.at/~mkoegler/pth/pthsem_2.0.8.tar.gz
    tar xzf pthsem_2.0.8.tar.gz
    cd pthsem-2.0.8
    dpkg-buildpackage -b -uc
    cd ..
    sudo dpkg -i libpthsem*.deb

    # now build+install knxd itself
    cd knxd
    git checkout v0.10 ## this branch
    dpkg-buildpackage -b -uc
    cd ..
    sudo dpkg -i knxd_*.deb knxd-tools_*.deb

### Daemon Configuration

Daemon configuration differs depending on whether you use systemd.
If "systemctl status" emits something reasonable, you are.

If you use systemd, the configuration file is ``/etc/knxd.conf``.
Socket activation is used for the default IP and Unix sockets
(port 6720 and /run/knx, respectively).

Without systemd, on Debian, edit ``/etc/default/knxd``.

The default Unix socket is ``/run/knx``.
Old eibd clients may still use ``/tmp/eib`` to talk to knxd.
You need to either change their configuration, or add "-u /tmp/eib"
to knxd's options.
(This is the default for "-u" before version 0.11. Don't depend on it.)

### Adding a TPUART USB interface

If you attach a (properly programmed) TUL (http://busware.de/tiki-index.php?page=TUL) to your computer, it'll show up as ``/dev/ttyACM0``.
This is a problem because (a) it's owned by root, thus knxd can't access it, and (b) if you ever add another serial interface that uses the same driver, knxd will use the wrong device.

Therefore, you do this:

* Run ``udevadm info --attribute-walk /sys/bus/usb/drivers/cdc_acm/*/tty/ttyACM0``.

  We're interested in the third block. It contains a line ``ATTRS{manufacturer}=="busware.de"``.
  Note the ``KERNELS=="something"`` line (your ``something`` will be different).

* Copy the following line to ``/etc/udev/rules.d/70-knxd.rules``:

  ```
  ACTION=="add", SUBSYSTEM=="tty", ATTRS{idVendor}=="03eb", ATTRS{idProduct}=="204b", KERNELS=="something", SYMLINK+="ttyKNX1", OWNER="knxd"
  ```

  Of course you need to replace the ``something`` with whatever ``udevadm`` displayed.
  An example file may be in ``/lib/udev/rules.d/``.

* Run ``udevadm test /sys/bus/usb/drivers/cdc_acm/*/tty/ttyACM0``.

* verify that ``/dev/ttyKNX1`` exists and belongs to "knxd":
  
  ``ls -lL /dev/ttyKNX1``

* add ``-b tpuarts:/dev/ttyKNX1`` to the options in ``/etc/knxd.conf``.

If you have a second TPUART, repeat with "ttyACM1" and "ttyKNX2".

You'll have to update your rule if you ever plug your TPUART into a different USB port.
This is intentional.


## Migrating from ``eibd``

* Before you even build: remove *any* traces of the old eibd installation from ``/usr/local``.
* The order of arguments is now significant. Among the "-D -T -R -S" arguments, ``-S`` must occur *last*.
* The groupswrite etc. aliases are no longer installed by default. To workaround, you can either add ``/usr/lib/knxd`` to your ``$PATH``, or use ``knxtool groupswrite``.
* If you use Debian Jessie or another systemd-based distribution, the ``/lib/systemd/system/knxd.socket`` file is used to open the local sockets for knxd. Therefore you no longer need your old ``-i`` or ``-u`` options.
* knxd's Unix socket should never have been located in ``/tmp``; the default is now ``/run/knx``.

## Contributions

* Any contribution is *very* welcome
* Please use Github and create a pull request with your patches
* Please see SubmittingPatches to correctly Sign-Off your code and add yourself to AUTHORS (`tools/list_AUTHORS > AUTHORS`)
* Adhere to our [coding conventions](https://github.com/knxd/knxd/wiki/CodingConventions).

## Community

* Code-related issues are on GitHub: https://github.com/knxd/knxd/issues
* For everything else there's a Google Groups forum on https://groups.google.com/forum/#!forum/knxd

