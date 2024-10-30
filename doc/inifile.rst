=================
INI file sections
=================

This document describes knxd's configuration file format.

General considerations
======================

INI-style files generally look like this::

    [section-name]
    option=value
    another-option=42
    bool-option=true

Recognized boolean values are "1", "y" or "true" vs. "0", "n" or "false".

An option ``use=foo`` may be used to pull in options from another section.

knxd must be called with one or two arguments:

* config file

  The name of the config file to read. '-' is used for standard input.

  There is no default.

* main section

  The section to use for the KNX router itself.
  
  The default is "main".

Unrecognized options are fatal. Unused sections are not.

Command-line options
--------------------

Previous versions of knxd used position-dependent options instead of a
configuration file. These are now deprecated. A translation program,
typically installed as ``/usr/lib/knxd_args``, will emit a correct
configuration file when called with the old knxd's arguments. ``knxd``
automatically calls ``knxd_args`` when it is started with "old" arguments
and interprets the resulting file. This may cause error messages to refer
to strange options instead of command-line arguments.

The following documentation mentions the old arguments. Note that
argument order is significant. All filters in front of -e|--eibaddr apply
globally; filters after that apply to the driver or server following them.
Filters after the last driver or server, apply to the systemd-socket-passed
server(s). Modifiers like ``--arg=name=value`` apply to the filter, driver or
server they're directly in front of.

As an example, these arguments::

    knxd_args -e 1.2.3 -E 1.2.4:5 -D --arg=delay=50 -B pace -R --layer2=log -T -S -b tpuarts:/dev/ttyACM0 -f 9 -t 252 -B log

generate this configuration file (re-ordered for clarity)::

    [main]
    addr = 1.2.3
    client-addrs = 1.2.4:5
    connections = server,B.tpuarts
    systemd = systemd

    [B.tpuarts]
    device = /dev/ttyACM0
    driver = tpuart

    [server]
    server = ets_router
    discover = true
    debug = debug-server
    router = router
    tunnel = tunnel

    [debug-server]
    name = mcast:knxd

    [router]
    filters = A.pace

    [A.pace]
    delay = 50
    filter = pace

    [tunnel]
    filters = log

    [systemd]
    debug = debug-systemd
    filters = log

    [debug-systemd]
    error-level = 0x9
    trace-mask = 0xfc

The config file generation code tries to keep the generated file reasonably
simple. Thus the "log" filters (unlike the "pace" filter) don't have a
section of their own, because they don't have arguments.

At startup, knxd will set up all configured drivers (unless they're
marked with ``ignore``).
If a driver fails, knxd will terminate with an error.
Use the ``retry`` filter to modify this.


main
====

The main section controls configuration of the whole of knxd. Its name
defaults to "main". An alternate main section may be named on the command
line.

* name (string; ``-n|--Name``)

  The name of this server. This name will be used in logging/trace output.
  It's also the default name used to announce knxd on multicast.

  Optional; defaults to "knxd".

* addr (string: KNX device address; ``-e|--eibaddr``)

  The KNX address of knxd itself. Used e.g. for requests originating at the
  group cache.

  Example: 1.2.3

  Mandatory.

* client-addrs (string: KNX device address plus length; ``-E|--client-addrs``)

  Address range to be distributed to client connections. Note that the
  length parameter indicates the number of addresses to be allocated.

  Example: 1.2.3:5

  (This assigns addresses 1.2.3 through 1.2.7 to knxd's clients.)

  Mandatory if you use knxd's server code (if in doubt: yes, you do).

* connections (string: list of section names; ``-b|--layer2 …``)

  Comma-separated list of section names. Each named section describes
  either a device to exchange KNX packets with, or a server which a
  remote device or program may connect to.

  Mandatory, as knxd is useless without any connections.

  On the command line, a --layer2= option compiles all preceding filter
  and debug parameters into a config file section. This also applies to
  the options --eibaddr (global data), --listen-tcp, --listen-local,
  --Routing, --Tunnelling, --Server, and --Groupcache.

* systemd (string: section name; implied at the end of the argument list)

  Section name for describing connections managed by ``systemd.socket``.

  The named section may be missing or empty::

      [main]
      systemd=systemd

      [systemd]
      error-level=fatal

* cache (string: section name; ``-c|--GroupCache``)

  Section name for the group cache. See the `group cache` section at the
  end of this document for details.

  Optional; mostly-required if you have a GUI that accesses KNX.

* force-broadcast (bool; ``--allow-forced-broadcast``)

  Packets have a "hop count", which determines how many routers they may
  traverse until they're discarded. This mitigates the problems caused by
  bus loops (routers reachable by more than one path).

  A maximum hop count is specified to (a) never be decremented, (b) such
  packets are broadcast to every interface instead of just those their
  destination address says they should go to.

  knxd ignores this requirement unless you set this option, because it's
  almost never useful and escalates configuration errors from "minor
  annoyance" to "absolute disaster if such a packet ever gets tramsmitted".

  Optional; default false.

* unknown-ok (bool; ``-A|--arg=unknown-ok=true``)

  Mark that arguments ``knxd`` doesn't know would emit a warning instead
  of being fatal. This is useful if you want a config file that allows
  for backwards compatibility.

  Optional; default false, to catch typos and thinkos.

* stop-after-setup (bool; ``-A|--arg=stop-after-setup=true``)

  Usually, knxd exits if there are any fatal configuration errors. 
  This option will instruct it to terminate even if there are no errors.
  You can thus use this option to verify that a configuration parses
  correctly.

  Optional; default false, also available as a command-line option.

Non-systemd options
-------------------

These options will be ignored when you start knxd with systemd.

* pidfile (string: file name; ``-p|--pid-file=FILENAME``)

  File to write knxd's process ID to.

  Optional, default: false.

* background (bool; ``-d|--daemon``)

  Instructs knxd to fork itself to the background.

  Optional, default: false.

* logfile (string: file name; ``-d|--daemon=FILENAME``)

  Tells knxd to write its output to this file instead of stderr.

  Optional, default: /dev/stderr.
  
  The file is closed and re-opened when you send a SIGHUP signal to knxd.
  
  On the command line, this option implied forking to the background.

Debugging and logging
=====================

You can selectively enable logging or tracing.

* debug (string: section name; implicit when using ``--error=`` or ``--trace=``)

  This option, available in all sections, names the config file section
  where specific debugging options for this section can be configured.

  Optional; if missing, read debug options from the current section, or
  from the main section.

A "debug" section may contain these options:

* error-level (string or int; ``-f|--error=LEVEL``)

  The minimum severity level of error messages to be printed.

  Possible values are 0…6, corresponding to none fatal error warning note info debug.

  Optional; default: warning.

* trace-mask (int; ``-t|--trace=MASK``)

  A bitmask corresponding to various types of loggable messages to help
  tracking down problems in knxd or one of its devices.

  For the meaning of possible values, read the source code. Notable bit
  positions:

  * 0

    byte-level tracing.

  * 1 

    Packet-level tracing.

  * 2

    Driver state transitions.

    This level used to also be used for logging KNX packets in a
    human-readable way; this has been subsumed by the "log" filter.

  * 3

    Dispatcher state transitions.

    The dispatcher (src/libserver/router.cpp) is the heart of knxd. it
    controls all drivers and decides to which drivers to re-distribute
    incoming packets.

  * 4

    Anything that's not a driver and talks directly to the dispatcher
    (group cache, high-level functions exposed by knxd's internal
    protocol). Also some more dispatcher states.

  * 6

    Debugging of flow control issues.

  Optional; default: no tracing.

  Thus, if you want to debug packet-level tracing and flow control, you'd
  use ``trace-mask=0x42``. If you don't know what is happening and want a
  full debug log, ``0xffe`` is often useful.

  .. Note::

      "packet-level tracing" does *not* include the data / control messages
      that are exchanged between the KNXD core and one of its drivers. You
      need to also add ``-B log`` if you want to see what's going on.
      
      This is not the default because you often need different subsets for
      verbosity and messaging.
 
.. Note::

    TODO: decide on a reasonable set of message types and allow selecting them
    by name.

* timestamps (bool; ``--no-timestamp``)

  Flag whether messages should include timestamps (since the start of knxd).

  You may turn these off when your logging system already reports with
  sufficient granularity or when you require reproducible logging output
  for tests.

  Optional; default: true.

The defaults are also used when no debug section exists.

Drivers
=======

A driver is a link to a KNX interface or router which knxd establishes when
it starts up. (In contrast, a server (below) is a point of connection which
knxd establishes so that other interfaces, routers or clients may connect
to it.)

Each interface in your "main" section names a section where that
interface's driver is configured. If a driver doesn't need any
configuration you may just use the name of the driver. Thus::

    [main]
    connections=foo,…

and::

    [main]
    connections=my-driver,…

    [my-driver]
    driver=foo

are equivalent, as are::

    [main]
    connections=my-driver,…

    [my-driver]
    driver=foo
    some-options=true

and::

    [main]
    connections=foo,…

    [foo]
    some-options=true

On the command line, driver options used to be added after the driver name,
separated by colons. The order of options reflects this. For example, the
driver "tpuart" accepted "device" and "baudrate" options; this command line::

    -b tpuarts:/dev/ttyACM0:19200

would be translated to a section::

    [X.tpuarts]
    driver=tpuart
    device=/dev/ttyACM0
    baudrate=19200

Common options
--------------

These options apply to all drivers and servers.

* ignore (bool; ``--arg=ignore=true``)

  The driver is configured, but not started up automatically.

  *Note*: Starting up knxd still fails if there is a configuration error.

dummy
-----

This driver discards all packets.

It does not have any options.

ip
--

This driver attaches to the IP multicast system. It is a minimal version of
the ``ets_router`` server's routing code (no tunnel server, no autodiscovery).

.. Warning::

    **Never** use this driver and the ``ets_router`` server at the same time (unless you
    specify different multicast addresses).

* multicast-address (string: IP address)

  The multicast IP address to use.

  Optional; the default is 224.0.23.12.

* port (int)

  The UDP port to listen on / transmit to.

  Optional; the default is ``3671``.

* interface (string: interface name)

  The IP interface to use.

  Optional; the default is the first broadcast-capable interface on your
  system, or the interface which your default route uses.

.. Note::

    You **must** use a multicast address here. Direct links to Ip
    interfaces are called "tunnels" and accessed with the ``ipt`` driver,
    below.

ipt
---

This driver is a tunnel client, i.e. it attaches to a remote tunnel server.
Hardware IP interfaces frequently use this feature.

You may need the "single" filter in front of this driver.

Note that some of these devices exhibit an implementation bug: after 3.5
weeks an internal timer overruns (the number of milliseconds exceeds 2³¹).
If this happens, add a "retry-delay" option to cause knxd to reconnect
instead of terminating.

* ip-address (string: IP address)

  The address (or host name) of the tunnel server to connect to.

  Mandatory.

* dest-port (int)

  The port to send to.
  
  Optional; the default is 3671.

* src-port (int)

  The port to send from.

  Optional; by default, the OS will assign a free port.

* nat (bool; implied by using ``iptn`` instead of ``ipt``)

  Require network address translation.

  TODO: when would you need that?

* heartbeat-timer (int; seconds)

  Timer for periodically checking whether the server is still connected
  to us.

  The default is 30.

* heartbeat-retries

  Retry timer for coping with lost heartbeat packets.

  The default is 3. If more consecutive heartbeat packets are unanswered,
  the interface will be considered failed.

The following options are not recognized unless "nat" is set.

* nat-ip (string: IP address)

  ??
  
  Mandatory if "nat" is set, otherwise disallowed.

* data-port (int)

  ??
  
  Mandatory if "nat" is set, otherwise disallowed.

.. Note::

    You **must not** use a multicast address here. Using multicast links
    is called "routing"; multicast is accessed either with the ``ip``
    driver, above, or the ``ets_router`` server, below.

iptn
----

See ``ipt``.

usb
---

This driver talks to "standard" KNX interfaces with USB. These interfaces
use the HID protocol, which is almost but not quite entirely unsuitable for
KNX but has the advantage that you can plug such an adapter into any
Windows computer and start ETS, without installing a special driver.

Usually, you do not need any options unless you have more than one of these
interfaces or it has non-standard configuration, as knxd will find it by itself.

You may need the "single" filter in front of this driver.

You may need a UDEV rule that changes the USB device's ownership to knxd.

These devices use one of three related protocols: EMI1, EMI2, or CEMI. The
driver auto-detects wvich version to use.

Warning: bus+device numbers may change after rebooting.

* bus (int)

  The USB bus the interface is plugged into.

* device (int)

  The interface's device number on the bus.

  It's an error to specify this option without also using "bus".

* config (int)

  The USB configuration to use on this device. Most interfaces only have
  one, so this option is not needed.

  It's an error to specify this option without also using "device".

* setting (int)

  The setting to use on this device configuration. Most interfaces only
  have one, so this option is not needed.

  It's an error to specify this option without also using "config".

* interface (int)

  The interface to use on this setting. Most interfaces only
  have one, so this option is not needed.

  It's an error to specify this option without also using "setting".

* version

  The EMI protocol version to use (1, 2, or 3).

  Default: None, the protocol to be used is auto-detected.

The following options control repetition of unacknowledged packets. They
also apply to the "ft12" and "ft12cemi" drivers which wrap EMI1 / CEMI data
in a serial protocol.

* send-timeout (int; ``--send-timeout=MSEC``)

  USB devices confirm packet transmission. This option controls how long
  to wait until proceeding. A warning is printed when that happens.

  The default is 300 milliseconds.

  Note that this driver's old "send-delay" option is misnamed, as the
  timeout is pre-emted when the remote side signals that is has accepted
  the packet.

* send-retries (int)

  The number of times to repeat the transmission of a packet. If
  (ultimately) unsuccessful, the packet will be discarded.

  The default is 3.

tpuart
-------

A TPUART or TPUART-2 interface IC. These are typically connected using either
USB or (on Raspberry Pi-style computers) a built-in 3.3V serial port.

* device (string: device file name)

  The device to connect to.

  Optional; the default is /dev/ttyKNX1 which is a symlink created by a
  udev rule, which you need anyway in order to change the device's owner.

* baudrate (int)

  Interface speed. This is interface specific, and configured in hardware.

  Optional; the default is 19200.

* low-latency (bool)

  Try to set serial latency even lower.

  This defaults to off because it doesn't work on every serial interface.

Alternately you can use::

    socat TCP-LISTEN:55332,reuseaddr /dev/ttyACM0,b19200,parenb,raw

on a remote computer, and connect to it via TCP. The options to use are

* ip-address=REMOTE

  The IP address (or host name) of the system ``socat`` runs on.

  Mandatory.

* dest-port=PORTNR

  The TCP port to connect to.

  Mandatory; use whatever free port you used on ``socat``'s command line.

On the command line, these drivers are called "tpuarts" and "tpuarttcp", for
compatibility with earlier versions.

ft12
----

An older serial interface to KNX which wraps the EMI1 protocol in serial framing.

TODO: which devices use this?

* baudrate (int)

  Interface speed. This is interface specific, and configured in hardware.

  Optional; the default is 19200.

* low-latency (bool)

  Try to set serial latency even lower.

  This defaults to off because it doesn't work on every serial interface.

* send-timeout (``--arg=send-timeout=MSEC``)

  EMI1 devices confirm packet transmission. This option controls how long
  to wait until proceeding. A warning is printed when that happens.

  The default is 0.3 seconds.

As with "tpuart", this device can be used remotely. On the command line,
the driver's name is "ft12tcp". Use this command on the remote side::

    socat TCP-LISTEN:55332,reuseaddr /dev/ttyACM0,b19200,parenb,raw

The "send-timeout" and "send-retries" options, described above on the "usb"
driver, can also be applied to this driver.

ft12cemi
--------

A newer serial interface to KNX.

TODO: which devices use this?

* baudrate (int)

  Interface speed. This is interface specific, and configured in hardware.

  Optional; the default is 19200.

* low-latency (bool)

  Try to set serial latency even lower.

  This defaults to off because it doesn't work on every serial interface.

* send-timeout (int; ``--arg=send-timeout=MSEC``)

  CEMI devices confirm packet transmission. This option controls how long
  to wait until proceeding. A warning is printed when that happens.

  The default is 0.3 seconds.

As with "tpuart", this device can be used remotely. On the command line,
the driver's name is "ft12cemitcp". Use this command on the remote side::

    socat TCP-LISTEN:55332,reuseaddr /dev/ttyACM0,b19200,parenb,raw

The ``send-timeout`` and ``send-retries`` options, described above on the "usb"
driver, can also be applied to this driver.

ncn5120
-------

A mostly-TPUART2-compatible KNX interface IC.

This driver uses the same options as "tpuarts". Its default baudrate is
38400.

As with "tpuart", this device can be used remotely. On the command line,
the driver's name is "ncn5120tcp". Use this command on the remote side::

    socat TCP-LISTEN:55332,reuseaddr /dev/ttyACM0,b38400,raw

More common options
-------------------

Some drivers accept these options.

* ack-group (bool; ``--tpuarts-ack-all-group``)

  Accept all group-addressed packets, instead of checking which knxd can
  forward. This option is usually a no-op because knxd forwards all
  packets anyway.

  This option only applies to drivers which directly connect to a
  twisted-pair KNX wire.

  Optional; default false.

* ack-individual (bool; ``--tpuarts-ack-all-individual``)

  Accept all device-addressed packets, instead of checking which knxd can
  forward. This option is not a no-op because, while knxd defaults to
  forwarding all packets, it won't accept messages to devices that it
  knows to be on the bus on which the message in question arrived.

  This option only applies to drivers which directly connect to a
  twisted-pair KNX wire.

  Optional; default false.

* reset (bool; ``--tpuarts-disch-reset``)

  Reset the device while connecting to it. This also affects
  reconnections due to timeout.

  Optional; default false.

* monitor (bool; ``--no-monitor``)

  Use this device as a bus monitor.

  When this option is set, no data will be sent to or accepted from this device.
  It will be set to bus-monitor mode and all incoming messages will only
  be forwarded to bus-monitoring clients.

  Optional; default false.

  If you want to monitor a specific device while using it normally, use
  the "monitor" filter instead.

  If you want to log all packets passing through knxd, use the
  "vbusmonitor" commands instead.

  There is no way to switch between bus monitoring and normal mode.
  This is intentional.

  Before v0.12, knxd did not adequately distinguish between monitoring
  and normal operation; instead, it switched the first interface without a
  --no-monitor option to monitoring whenever a client wanted a bus
  monitor. This no longer happens.

Servers
=======

A server is a point of connection which knxd establishes so that other
interfaces, routers or clients may connect to it. (In contrast, a driver is
a link to a KNX interface or router which knxd establishes when it starts up.)

Common options
--------------

See the "Common options" section under "Drivers", above.

ets_router
----------

The "ets_router" server allows clients to discover knxd and to connect to it
with the standardized KNX tunneling or routing protocols.

.. Warning::

    **Never** use this server and the ``ip`` driver at the same time (unless you
    specify different multicast addresses).

* tunnel (str; ``-T|--Tunnelling``)

  Allow client connections via tunneling. This is typically used by
  single devices or programs.

  This option names a section with configuration for tunnelled
  connections. It's OK if that section doesn't exist or is empty.

  Optional; tunneling is disabled if not set.

* router (str; ``-R|--Routing``)

  Exchange packets via multicast. This is typically used by other KNX
  routers.

  This option names a section with configuration for the multicast
  connection. It's OK if that section doesn't exist or is empty.

  Optional; multicast is disabled if not set.

* discover (bool; ``-D|--Discovery``)

  Reply to KNX discovery packets. Programs like ETS send these packets to
  discover routers and tunnels.

  Optional; default false.

* multi-port (bool; ``--multi-port`` / ``--single-port``)

  If set, instructs knxd to use a separate port for exchanging KNX data
  instead of using the default port. This allows two KNX routers (knxd or
  otherwise) to co-exist on the same computer.

  Unfortunately, using a single port is so common that some programs,
  like ETS, ignore packets from a different port, even if that port is
  announced in the discovery phase.

  Optional; default false (for now).

* interface (string; 3rd option of ``-S``/``--Server``)

  The IP interfce to use. Useful if your KNX router has more than one IP
  interface.

  Optional; defaults to the interface with the default route.

* multicast-address (string: IP address; 1st option of -S/--Server)

  The multicast IP address to use.

  Optional; the default is 224.0.23.12.

* port (int; 2nd option of ``-S``/``--Server``)

  The UDP port to listen on / transmit to.

  Optional; the default is 3671.

* name (string; not available)

  The server name announced in Discovery packets.

  Optional: default: the name configured in the "main" section, or "knxd".

* heartbeat-timeout (integer: keep-alive timeout)

  The maximum time between status messages from tunnel clients. A client
  that doesn't send any packets for this long is disconnected.

On the command line, this server is typically used as "-DTRS". The
-S|--Server argument has to be used last and accepted the options mentioned
above.

tcptunsrv
----------

The "tcptunsrv" server allows clients to connect knxd using the KNXnet/ip
TCP tunneling protocol.

* tunnel (str)

  This option names a section with configuration for tunnelled
  connections. It's OK if that section doesn't exist or is empty.

* port (int)

  The TCP port to listen on / transmit to.

  Optional; the default is 3671.

* systemd-ignore (bool)

  Ignore this server when knxd is started via systemd.

  Optional; default "true" if listening on TCP port 3671.

* heartbeat-timeout (integer: keep-alive timeout)

  The maximum time between status messages from tunnel clients. A client
  that doesn't send any packets for this long is disconnected.

unixtunsrv
----------

The "unixtunsrv" server allows clients to connect knxd using the KNXnet/ip
TCP tunneling protocol over Unix domain sockets.

* tunnel (str)

  This option names a section with configuration for tunnelled
  connections. It's OK if that section doesn't exist or is empty.

* path (string: file name)

  Path to the socket file to use.

* systemd-ignore (bool)

  Ignore this server when knxd is started via systemd.

  Optional; default is "false"

* heartbeat-timeout (integer: keep-alive timeout)

  The maximum time between status messages from tunnel clients. A client
  that doesn't send any packets for this long is disconnected.

knxd_unix
---------

Allow local knxd-specific clients to connect using a Unix-domain socket.

* path (string: file name; 1st option to ``-u|--listen-local``)

  Path to the socket file to use.

  Optional; default /run/knx.

* systemd-ignore (bool; ``--arg=systemd-ignore=BOOL``)

  Ignore this option when knxd is started via systemd.

  Optional; default "true" if no path option is used.

knxd_tcp
--------

Allow remote knxd-specific clients to connect using a TCP socket.

* ip-address (string: IP address; 1st option to ``-i|--listen-tcp``)

  Bind to this address.

  Default: none, i.e. listen on all addresses the system is using.

* port (int; 2nd option to ``-i|--listen-tcp``)

  TCP port to bind to.

  Optional; default 6720.

* systemd-ignore (bool; ``--arg=systemd-ignore=BOOL``)

  Ignore this option when knxd is started via systemd.

  Optional; default "true" if no port option is used.

Filters
=======

A filter is a module which is inserted between the knx router itself and a
specific driver. You specify filters with a ``filters=`` option in the
driver's or server's section.

On the command line, ``-B|--filter=NAME`` told knxd to apply this filter to the
next-specified driver.

Each filter names a section where that filter is configured. If a filter
doesn't need any configuration you may just use the name of the filter.
Thus::

    -B foo -B bar -b some-driver

translates to::

    [some-driver]
    filters=foo,bar

which is equivalent to::

    [my-driver]
    driver=some-driver
    filters=my-foo,my-bar

    [my-foo]
    filter=foo

    [my-bar]
    filter=bar

For a more elaborate example::

    --arg=some-option --filter=foo --arg=an-option_a-value --layer2=some-driver

translates to::

    [my-driver]
    driver=some-driver
    filters=my-filter,…
    an-option=a-value

    [my-filter]
    filter=foo
    some-option=true

If you just use::

    -B foo -b some-driver

this can be simplified to::

    [some-driver]
    filters=foo

Of course, all of these examples also add the driver's section name
("my-driver" or "some-driver") to the ``connections=`` parameter in the main
section.

Filters are applied in order; conceptually, the knx router is added at the
beginning of the filter list, while the driver itself is at the end.
Transmitted packets pass that chain from beginning to end, while received
packets move the other way.

If you specify filters on a server, each driver that's started by the
server gets this set of filters.

single
------

This filter allows knxd to connect to devices which only expect (or accept)
a single device. Thus, on outgoing packets knxd will remember the sender's
address in order to re-address any replies (if they're addressed
individually).

* address (``--arg=address=N.N.N``)

  The "single" filter typically uses knxd's address. However, that
  address is also used for multicast and thus is on the wrong line.

  Thus, you can use this option to assign a different address.

If you use this filter behind an ``ipt:`` driver, the address it uses will be
replaced with the one assigned by the remote server.

remap
-----

This filter allows knxd to connect to devices which internally use
somewhat-random addresses. This may happen when a remote system reconnects
and re-uses its old address instead of the new one.
Thus, on incoming packets knxd will remember the sender's
address in order to re-address any replies (if they're addressed
individually).

This filter can also be used to connect remote networks with
physical addresses that collide with whatever is connected to the rest of
knxd.

Unlike the "single" filter, "remap" does not take an address parameter
because its whole point is to use the address assigned to the link by knxd.

retry
-----

If a driver fails, the default behavior is to terminate knxd.
You can use this module to restart the driver instead.

Also, this module can be used to restart a driver if transmission
of a message takes longer than some predefined time.

This module is transparently inserted in front of line drivers.

* max-retries (int, counter)

  The maximum number of retries when opening a driver or sending a packet.
 
  The default is one: if the first attempt fails, the error is propagated
  to the router. Set to zero if the number of retries should not be limited.

  For compatibility with older configuration, this option may be used
  on the driver.

* retry-delay (float, seconds)

  The time before (re-)opening the driver when a failure occurred.

  The default is zero: no timeout.

  For compatibility with older configuration, this option may be used
  on the driver.

* send-timeout (float, seconds)

  The time after which a message must have been transmitted.
  Otherwise the driver is closed and re-opened.

  The default is zero: no timeout.

  This option is also accepted on some drivers, but the semantics is
  different: the ``retry`` filter always restarts the driver before
  retrying, whereas the driver should assume a soft transmission error.

* start-timeout (float, seconds)

  The maximum time (re-)opening the driver may take.

  The default is zero: no timeout.

* flush (bool)

  Flag whether to-be-transmitted packets are discarded while the driver is down.

  The default is False: packet transmission will halt. You might want to use a ``queue``
  filter in front of ``retry``.

* may-fail (bool)

  Flag whether the initial start of this driver may fail, i.e. if set, this module
  affects the driver's initial start-up.
 
  The default is False, i.e. retrying only kicks in after the driver is operational.

  For compatibility with older configuration, this option may be used
  on the driver.

queue
-----

The normal behavior of knxd is to couple the transmission speed of all its
interfaces, so that packets are transmitted on all of them (if they request
them) at roughly the same speed, i.e. that of the slowest interface.

This filter implements a queue which decouples an interface, so that its
speed does not affect the rest of the system.

The "queue" filter does not yet have any parameters.

Currently, a queue can grow without bounds; it also does **not** filter
multiple packets to the same group address.

pace
----

Limit the rate at which packets are transmitted to an interface.

* delay (float, msec)

  The delay between transmissions.
  
  Optional. The default is 20 msec.

* delay-per-byte (float, msec)

  Additional delay per byte of longer messages, in milliseconds.
  
  Note that protocol headers and other overhead is ignored here.
  The fixed-overhead part of the KNX protocol must be factored into the
  ``delay`` parameter.

  Optional; the default is 1 msec, which roughly corresponds to one byte
  at 9600 baud (speed of the KNX bus).

* incoming (float, proportion)

  The "pace" filter only considers outgoing packets. However,
  since KNX is a bus, incoming data also need to be considered.

  This parameter controls whether incoming data should contribute to the
  filter's delay.

  Optional. The default is 0.75.

The pace filter's timer starts when a packet has successfully been
transmitted. Thus it should only be necessary in front of the multicast
driver (which does not have transmission confirmation). However, there are
buggy KNX interfaces out there which acknowledge reception of packets
*before* checking whether they have free buffer space for more data …

Note that knxd schedules packet transmission synchronously. Thus, this
filter acts globally (it delays sending to *all* interfaces) unless
there is a ``queue`` filter in front of it.

If you use this filter to rate-limit a sender by more than the recipient's
data rate, you should set the ``incoming`` parameter to zero. Otherwise a
high rate of incoming messages will block the sender entirely.

monitor
-------

This filter forwards all packets passing through it to knxd's bus monitoring
system.

* recv (bool; ``--arg=recv=BOOL``)

  Monitor incoming packets. Defaults to true.

* send (bool; ``--arg=send=BOOL``)

  Monitor outgoing packets. Defaults to true.


log
---

This filter logs all packets passing through it to knxd's logging system.

* name (string; ``--arg=name=NAME``)

  Set the output's name. The default is "logsection/driversection".

* recv (bool; ``--arg=recv=BOOL``)

  Log incoming packets. Defaults to true.

* send (bool; ``--arg=send=BOOL``)

  Log outgoing packets. Defaults to true.

* state (bool; ``--arg=state=BOOL``)

  Log state transitions (link up/down). Defaults to true.

* addr (bool; ``--arg=addr=BOOL``)

  Log address checks, i.e. whether the driver knows and/or accepts 
  a particular device or group address. Defaults to false.

* monitor (bool; ``--arg=monitor=BOOL``)

  Log bus monitor packets. Defaults to false.

dummy
-----

This filter does nothing.

Special settings
================

These are enabled by naming them in designated wntries of your main section.

Thus, you enable the group cache with::

    [main]
    cache=gc
    [gc]
    max-size=200

If you don't want to use any parameters, you don't need to add the section::

    [main]
    cache=gc

group cache
-----------

* max-size (int; option of ``-c|--GroupCache``)

  The maximum number of messages that the group cache will store.

  Optional; no default = no limit. There are 65535 possible group address
  entries which only occupy a few bytes each, so the recommended usage is
  to not specify a maximum.

  This is the optional parameter of the ``--GroupCache`` argument.

