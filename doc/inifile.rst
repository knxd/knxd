=================
INI file sections
=================

General considerations
======================

INI-style files generally look like this:

  [section-name]
  option=value
  another-option=42
  bool-option=true

Recognized boolean values are "1", "y" or "true" vs. "0", "n" or "false".

An option "use=foo" may be used to pull in options from another section.

knxd must be called with one or two arguments:

  * config file

    The name of the config file to read. '-' is used for standard input.

    There is no default.

  * main section

    The section to use for the KNX router itself.
    
    The default is "main".

Unrecognized options are fatal. Unused sections are not.

Thus if you need a config file to be recognized by more than one version of
knxd, use different "main" sections.

Command-line options
--------------------

Previous versions of knxd used position-dependent options instead of a
configuration file. These are now deprecated. A translation program,
typically installed as ``/usr/lib/knxd_args``, will emit a correct
configuration file when called with the old knxd's arguments.

main
====

The main section controls configuration of the whole of knxd. Its name
defaults to "main". An alternate main section may be named on the command
line.

  * name (string)

    The name of this server. This name will be used in logging/trace output.
    It's also the default name used to announce knxd on multicast.

    Optional; defaults to "knxd".

  * addr (string: KNX device address)

    The KNX address of knxd itself. Used e.g. for requests originating at the
    group cache.

    Example: 1.2.3

    Mandatory.

  * client-addrs (string: KNX device address plus length)

    Address range to be distributed to client connections.

    Example: 1.2.4:5

    Mandatory if you use the server code in knxd (if in doubt: yes, you do).

  * connections (string: list of section names)

    Comma-separated list of section names. Each named section describes
    either a device to exchange KNX packets with, or a server which a
    remote device or program may connect to.

    Mandatory, as knxd is useless without any connections.

  * systemd (string: section name)

    Section name for describing connections managed by ``systemd.socket``.

    The named section may be missing or empty.

        [main]
	systemd=systemd

	[systemd]
        error-level=fatal
	

  * cache (string: section name)

    Section name for the group cache.

    Optional; mostly-required if you have a GUI that accesses KNX.

  * force-broadcast (bool)

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

  * unknown-ok (bool)

    Mark that arguments ``knxd`` doesn't know whould emit a warning instead
    of being fatal. This is useful if you want a config file that allows
    for backwards compatibility.

    Optional; default false, to catch typos and thinkos.

  * stop-after-setup (bool)

    Usually, knxd exits if there are any fatal configuration errors. 
    This option will instruct it to terminate even if there are no errors.
    You can thus use this option to verify that a configuration parses
    correctly.

    Optional; default false, also available as a command-line option.

Non-systemd options
-------------------

These options will be ignored when you start knxd with systemd.

  * pidfile (string: file name)

    File to write knxd's process ID to.

    Optional, default: false.

  * background (bool)

    Instructs knxd to fork itself to the background.

    Optional, default: false.

  * logfile (string: file name)

    Tells knxd to write its output to this file instead of stderr.

    Optional, default: /dev/stderr.

Debugging and logging
=====================

You can selectively enable logging or tracing.

  * debug (string: section name)

    This option, available in all sections, names the config file section
    where specific debugging options for this section can be configured.

    Optional; if missing, read debug options from the current section, or
    from the main section.

A "debug" section may contain these options:

  * error-level (string or int)

    The minimum severity level of error messages to be printed.

    Possible values are 0…6, corresponding to none fatal error warning note info debug.

    Optional; default: warning.

  * trace-mask (int)

    A bitmask corresponding to various types of loggable messages to help
    tracking down problems in knxd or one of its devices.

    For the meaning of possible values, reasd the source code.

    Optional; default: no tracing.

  * timestamps (bool)

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
configuration you may just use the name of the driver. Thus,

    [main]
    interfaces=foo,…

and

    [main]
    interfaces=my-driver,…

    [my-driver]
    driver=foo

are equivalent, as are

    [main]
    interfaces=my-driver,…

    [my-driver]
    driver=foo
    some-options=true

and

    [main]
    interfaces=foo,…

    [foo]
    some-options=true

Common options
--------------

These options apply to all drivers and servers.

  * ignore (bool)

    The driver is configured, but not started up automatically.

    *Note*: Starting up knxd still fails if there is a configuration error.

  * may-fail (bool)

    The driver is started, but does not block starting to route packets.

  * retry (int)

    If the driver fails to start (or dies), knxd will restart it after this
    many seconds.

    Default: zero: no restart.

If "retry" is active but "may-fail" is false, the driver must start
correctly when knxd starts up. It will only be restarted once knxd is, or
rather has been, fully operative.

dummy
-----

This driver discards all packets.

It does not have any options.

ip
--

This driver attaches to the multicast system. It is a minimal version of
the "router" server's routing code (no tunnel server, no discovery).

Never use this driver and the "router" server on the same multicast
address.

  * multicast-address (string: IP address)

    The multicast IP address to use.

    Optional; the default is 224.0.23.12.
  
  * port (int)

    The UDP port to listen on / transmit to.

    Optional; the default is 3671.
  
  * interface (string: interface name)

    The IP interface to use.

    Optional; the default is the first broadcast-capable interface on your
    system, or the interface which your default route uses.

ipt
---

This driver is a tunnel client, i.e. it attaches to a remote tunnel server.
Hardware IP interfaces frequently use this feature.

You may need the "single" filter in front of this driver.

  * ip-address (string: IP address)

    The address (or host name) of the tunnel server to connect to.

    Mandatory.
  
  * dest-port (int)

    The port to send to.
    
    Optional; the default is 3671.
  
  * src-port (int)

    The port to send from.

    Optional; by default, the OS will assign a free port.

  * nat (bool)

    Require network address translation.

    TODO: when would you need that?

  * nat-ip (string: IP address)
  
    ??
    
    Mandatory if "nat" is set, otherwise disallowed.
  
  * data-port (int)

    ??
    
    Mandatory if "nat" is set, otherwise disallowed.
  
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

tpuarts
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

ft12
----

An older serial interface to KNX.

TODO: which devices use this?

ft12cemi
--------

A newer serial interface to KNX.

TODO: which devices use this?

ncn5120
-------

A mostly-TPUART2-compatible KNX interface IC.

This driver uses the same options as "tpuarts". Its default baudrate is
38400.

tpuarttcp
---------

A TPUART or TPUART-2 interface connected via a remote TCP socket.

  * ip-address (string)
  
    The remote system's IP address (or host name).

    Mandatory.

  * dest-port (int)

    The destination port to connect to.

    Mandatory.

More common options
-------------------

Some drivers accept these options.

  * ack-group (bool)

    Accept all group-addressed packets, instead of checking which knxd can
    forward. This option is usually a no-op because knxd forwards all
    packets anyway.

    This option only applies to drivers which directly connect to a
    twisted-pair KNX wire.

    Optional; default false.

  * ack-individual (bool)

    Accept all device-addressed packets, instead of checking which knxd can
    forward. This option is not a no-op because, while knxd defaults to
    forwarding all packets, it won't accept messages to devices that it
    knows to be on the same bus as the message in question.

    This option only applies to drivers which directly connect to a
    twisted-pair KNX wire.

    Optional; default false.

  * reset (bool)

    Reset the device while connecting to it. This also affects
    reconnectiosn due to timeout.

    Optional; default false.

  * monitor (bool)

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

Servers
=======

A server is a point of connection which knxd establishes so that other
interfaces, routers or clients may connect to it. (In contrast, a driver is
a link to a KNX interface or router which knxd establishes when it starts up.)

Common options
--------------

See the "Common options" section under "Drivers", above.

router
------

The "router" server allows clients to discover knxd and to connect to it
with the standardized KNX tunneling or routing protocols.

*Do not* use the "router" server and the "ip" driver at the same time.

  * tunnel (str)

    Allow client connections via tunneling. This is typically used by
    single devices or programs.

    This option names a section with configuration for tunnelled
    connections. It's OK if that section doesn't exist or is empty.

    Optional; tunneling is disabled if not set.

  * router (str)

    Exchange packets via multicast. This is typically used by other KNX
    routers.

    This option names a section with configuration for the multicast
    connection. It's OK if that section doesn't exist or is empty.

    Optional; multicast is disabled if not set.

  * discover (bool)

    Reply to KNX discovery packets. Programs like ETS send these packets to
    discover routers and tunnels.

    Optional; default false.

  * multi-port (bool)

    If set, instructs knxd to use a separate port for exchanging KNX data
    instead of using the default port. This allows two KNX routers (knxd or
    otherwise) to co-exist on the same computer.

    Unfortunately, using a single port is so common that some programs,
    like ETS, ignore packets from a different port, even if that port is
    announced in the discovery phase.

    Optional; default false (for now).

  * interface (string)

    The IP interfce to use. Useful if your KNX router has more than one IP
    interface.

    Optional; defaults to the interface with the default route.

  * multicast-address (string: IP address)

    The multicast IP address to use.

    Optional; the default is 224.0.23.12.
  
  * port (int)

    The UDP port to listen on / transmit to.

    Optional; the default is 3671.

  * name (string)

    The server name announced in Discovery packets.

    Optional: default: the name configured in the "main" section, or "knxd".

unix-socket
-----------

Allow local knxd-specific clients to connect using a Unix-domain socket.

  * path (string: file name)

    Path to the socket file to use.

    Optional; default /run/knx.

  * systemd-ignore

    Ignore this option when knxd is started via systemd.

    Optional; default "true" if no path option is used.

tcp-socket
----------

Allow remote knxd-specific clients to connect using a TCP socket.

  * ip-address (string: IP address)

    Bind to this address.

    Default: none, i.e. listen on all addresses the system is using.

  * port (int)

    TCP port to bind to.

    Optional; default 6720.

  * systemd-ignore

    Ignore this option when knxd is started via systemd.

    Optional; default "true" if no port option is used.

Filters
=======

A filter is a module which is inserted between the knx router itself and a
specific driver. You specify filters with a "filters=" option in the
driver's or server's section.

Each filter names a section where that filter is configured. If a filter
doesn't need any configuration you may just use the name of the filter.
Thus,

    [some-driver]
    filters=foo,…

and

    [some-driver]
    filters=my-filter,…

    [my-filter]
    filter=foo

are equivalent, as are

    [some-driver]
    filters=my-filter,…

    [my-filter]
    filter=foo
    some-option=true

and

    [some-driver]
    filters=foo,…

    [foo]
    some-option=true

Filters are applied in order; conceptually, the knx router is added at the
beginning of the filter list, while the driver itself is at the end.

If you specify filters on a server, each driver that's started by the
server gets this set of filters.

single
------

This filter allows knxd to connect to devices which only expect (or accept)
a single device. Thus, on outgoing packets knxd will remember the sender's
address in order to re-address any replies (if they're addressed
individually).

The "single" filter may not be necessary unless you're programming devices with ETS.

queue
-----

The normal behavior of knxd is to couple the transmission speed of all its
interfaces, so that packets are transmitted on all of them (if they request
them) at roughly the same speed, i.e. that of the slowest interface.

This filter implements a queue which decouples an interface, so that its
speed does not affect the rest of the system.

The "queue" filter does not yet have any parameters.

pace
----

Limit the rate at which packets are transmitted to an interface.

  * delay

    The delay between transmissions, in milliseconds.
    
    Mandatory.

Note that this filter acts globally, i.e. delays transmission to *all*
interfaces, if there is no queue in front of it.

monitor
-------

This filter forwards all packets passing through it to knxd's bus monitoring
system.

TODO.

log
---

This filter logs all packets passing through it to knxd's logging system.

  * name

    Set the output's name. The default is "log".

  * recv

    Log incoming packets. Defaults to true.

  * send

    Log outgoing packets. Defaults to true.

  * state

    Log state transitions (link up/down). Defaults to true.

  * addr

    Log address checks, i.e. whether the driver knows and/or accepts 
    a particular device or group address. Defaults to false.

  * monitor

    Log incoming bus monitor packets. Defaults to false.

dummy
-----

This filter does nothing.

Special settings
================

These are enabled by naming them in designated wntries of your main section.

Thus, you enable the group cache with

  [main]
  cache=gc
  [gc]
  max-size=200

If you don't want to use any parameters, you don't need to add the section:

  [main]
  cache=gc

group cache
-----------

  * max-size

    The maximum number of messages that the group cache will store.

    Optional; no default = no limit. There are 65535 possible group addresses
    entries, so the recommended usage is to not specify a maximum unless
    knxd is running on an embedded system.

