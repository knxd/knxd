knxd [![Build Status](https://travis-ci.org/knxd/knxd.svg)](https://travis-ci.org/knxd/knxd)
====

KNX is a very common building automation protocol which runs on dedicated 9600-baud wire as well as IP multicast.
``knxd`` is an advanced router/gateway which runs on any Linux computer; it can talk to all known KNX interfaces.

This code is a fork of eibd 0.0.5 (from bcusdk)
https://www.auto.tuwien.ac.at/~mkoegler/index.php/bcusdk

For a (german only) history and discussion why knxd emerged please also see: [eibd(war bcusdk) Fork -> knxd](http://knx-user-forum.de/forum/öffentlicher-bereich/knx-eib-forum/39972-eibd-war-bcusdk-fork-knxd)

# Future stable version

This version should be OK for general use.

Check [the Wiki page](https://github.com/knxd/knxd/wiki) for other version(s) to use.

## Known bugs

* ETS programming has not yet been tested

## New Features since 0.12

### see https://github.com/knxd/knxd/blob/v0.12/README.md for earlier changes

* 0.14

  * Configuration

    * There are no longer separate --enable-tpuarts and --enable-tpuarttcp
      options. Instead, you control both with --enable-tpuart. (This is the
      default anyway.)

  * Configuration file

    * includes a translator (knxd\_args) from options to config file
    
    * Most (if not all) settings are still usable via the command line

  * Complete stack refactored

    * You may now use global filters.

    * USB handling updated

    * Most device-specific drivers are now split into a top part which
      translates KNX packets to wire format (usually CEMI), and a bottom
      part which transmits/receives the actual data. This enables extensive
      code sharing; knxd also can use TCP connections instead of actual
      serial devices.

  * Startup sequencing fixed: KNX packets will not be routed
    until all interfaces are ready.

    Also, systemd will not be signalled until then.

    * Configuration options to not start, or start and ignore failures of,
      specific interfaces

    * knxd will now retry setting up an interface

  * use libfmt for sane and type-safe formatting of error and trace messages

  * logging packets is now done with a filter

    * packet-level "logging" calls in various drivers have been removed

  * Complain loudly (and early) if knxd needs -E / client-addrs=X.Y.Z:N

  * There is now a "log" filter. Logging of complete packets
    (inconsistently bit 1, 2, or 8 of the tracing mask) has been removed
    from individual drivers.

    This also applies to global packet logging.

  * knxd can restart links when they fail, or start to come up.

  * Interfaces are now either used normally, or in bus monitor mode.
    This is set in the configuration file / on the command line.
    There is no longer a way to switch between these modes;
    "knxtool busmonitor" will no longer change the state of any interface.

  * Queuing and flow control.

    Previously, all drivers implemented their own queueing for
    outgoing packets, resulting in duplicate code and hidden errors.

    In v0.14, the main queueing system will pace packets for the slowest device.
    If you don't want that, use the "queue" filter on the slow device(s).

    Output queues in drivers have been removed.

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

## Migrating to 0.14

* knxd is now configured with a .ini-style configuration file.

  The old way of configuring knxd via a heap of position-dependent
  arguments is still supported.

  You can use ``/usr/lib/knxd_args <args-to-knxd>`` to emit a file that
  corresponds to your old list of arguments.

* Not configuring client addresses for the knxd\_\* servers (options -i -u),
  systemd sockets, or the router's tunnel mode (-T) now results in that
  service not being offered at all, instead of only failing when a client
  connects. An error message is emitted. The multicast server will not
  start at all, and systemd startup will fail.

* knxd will not start routing any packets unless startup is successful on
  all interfaces.

  This means that it is now safe to use "socket activation" mode with
  systemd. Previously, knxd might have lost the initial packets.

* Tracing no longer logs the actual decoded contents of packet.
  If you need that, use the "log" filter.

* knxd now transmits data synchronously, i.e. individual drivers no longer
  buffer data for transmission. If you don't want that, use the "queue"
  filter on slow interfaces.

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

