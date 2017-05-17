knxd [![Build Status](https://travis-ci.org/knxd/knxd.svg)](https://travis-ci.org/knxd/knxd)
====

KNX is a very common building automation protocol which runs on dedicated 9600-baud wire as well as IP multicast.
``knxd`` is an advanced router/gateway which runs on any Linux computer; it can talk to all known KNX interfaces.

This code is a fork of eibd 0.0.5 (from bcusdk)
https://www.auto.tuwien.ac.at/~mkoegler/index.php/bcusdk

For a (german only) history and discussion why knxd emerged please also see: [eibd(war bcusdk) Fork -> knxd](http://knx-user-forum.de/forum/öffentlicher-bereich/knx-eib-forum/39972-eibd-war-bcusdk-fork-knxd)

# Unstable development version!

This version is unstable. Development happens here.

Please switch to the v0.12 branch for productive use.

    git checkout v0.12

## New Features since 0.12

### see https://github.com/knxd/knxd/blob/v0.12/README.md for earlier changes

* 0.13 (development)

  * TBD

  * TPUART support rewritten (single state machine).
  * Support for TPUART-2 added
    * auto-ACK mode auto-enabled when "single" filter is used

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

    # now build+install knxd
    cd knxd
    git checkout master
    dpkg-buildpackage -b -uc
    # To repeat: if this fails because of missing dependencies,
    # fix them instead of using dpkg-buildpackage's "-d" option.
    cd ..
    sudo dpkg -i knxd_*.deb knxd-tools_*.deb

    # … and if you'd like to update knxd:
    rm knxd*.deb
    cd knxd
    git pull
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
(This was the default for "-u" before version 0.11.)

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

### Adding a TPUART serial interface to the Raspberry Pi

The console is /dev/ttyAMA0. The udev line is

  ```
  ACTION=="add", SUBSYSTEM=="tty", KERNELS="ttyAMA0", SYMLINK+="ttyKNX1", OWNER="knxd"
  ```

This rule creates a symlink /dev/ttyKNX1 which points to the console. The
knxd configuration will use that symlink.

You need to disable the serial console. Edit ``/boot/cmdline.txt`` and
remove the ``console=ttyAMA0`` entry. Then reboot.

On the Raspberry Pi 3, the serial console is on ``ttyAMA1`` by default.
However, that is a software-driven serial port (the hardware serial
interface is used for Bluetooth on the Pi3). Varying CPU speed causes this
port to be somewhat unreliable. If this happens, disable bluetooth by adding

  ```
  dtoverlay=pi3-disable-bt
  ```

to ``/boot/config.txt``, executing ``systemctl disable hciuart``, and
rebooting. The TPUART module is now back on ``ttyAMA0``.


## Migrating to 0.12

* If you build knxd yourself: install the ``libev-dev`` package.
  You no longer need the ``pthsem`` packages.

* You may need "-B single" in front of any "-b ipt:" or "-b usb:", esp.
  when you need to program a device; normal use is often not affected.
  knxd emits a warning
  
  ``Message without destination. Use the single-node filter ('-B single')?``

  when it detects mis-addressed packets.

* You need "-e"; knxd no longer defaults to address 0.0.1.

* You need "-E" if you want to allow clients to connect (options -u -i -T).
  As that's almost always the case, knxd will print a warning if this
  option is missing.

* If you use knxtool's management tools (any command with "progmode" or
  whose name starts with 'm'), please [open an issue](https://github.com/knxd/knxd/issues)
  because knxd currently does not support these commands.

## Migrating from ``eibd``

* Before you build knxd: remove *any* traces of the old eibd installation
  from ``/usr/local``, or wherever you installed it.

* The order of arguments is now significant. Among the "-D -T -R -S" arguments, ``-S`` must occur *last*.
  Arguments which modify the behavior of an interface must be in front
  of that interface. Global arguments (e.g. tracing the datagram router)
  must be in front of the "-e" option.

* The 'groupswrite' etc. aliases are no longer installed by default. To
  workaround, you can either add ``/usr/lib/knxd`` to your ``$PATH``, or
  use ``knxtool groupswrite``.

* If you use Debian Jessie or another systemd-based distribution,
  ``/lib/systemd/system/knxd.socket`` is used to open the "standard"
  sockets on which knxd listens to clients. You no longer need your old
  ``-i`` or ``-u`` options.

* knxd's Unix socket should never have been located in ``/tmp``; the
  default is now ``/run/knx``. You can add a "-u /tmp/eib" (or whatever)
  option if necessary, but it's better to fix the clients.

## Contributions

* Any contribution is *very* welcome
* Please use Github and create a pull request with your patches
* Please see SubmittingPatches to correctly Sign-Off your code and add yourself to AUTHORS (`tools/list_AUTHORS > AUTHORS`)
* Adhere to our [coding conventions](https://github.com/knxd/knxd/wiki/CodingConventions). The git archive includes a helpful .vimrc file if you use VIM.

### Compensation – personal statement

KNX development is not a simple matter and requires both time and dedicated
hardware for tests. The ETS software isn't exactly cheap, either, and
there is no free replacement. (I'd like to change that.)

Thus, wearing my hat as the (current) main author, I (Matthias Urlichs)
would like to ask you to consider contributing to knxd's development.

* paypal: matthias@urlichs.de
* bitcoin: 1G2NKavCVt2adxEUZVG437J2tHvM931aYd
* SEPA: DE25760400610535260401 @ COBADEFFXXX

I can issue a commercial invoice if required.

If you'd rather gift some hardware, please ask.

## Community

* Code-related issues (aka "bugs") are on GitHub: https://github.com/knxd/knxd/issues
* For everything else there's a Google Groups forum on https://groups.google.com/forum/#!forum/knxd

