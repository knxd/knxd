=====================
The new KNXD protocol
=====================

This document outlines the new protocol which clients of ``knxd`` should
use to talk to it.

This document is incomplete. Specifically, it actcually describes version 1.
Versions 0.x will not actually implement some of the features described here.
This paragraph will be removed when the still-missing parts are suitably
annotated.

The Problem
===========

``knxd`` uses a private protocol on port 6720. This protocol is
undocumented, does not support concurrent requests, and exposes several
features which depended on ``libpthsem``; thus they have not been
re-implemented in the current version. Also, while knxd's source includes
auto-generated implementations of this protocol for various scripting
languages, these have not been tested well (if at all) and frequently are
not idiomatic, forcing the programmer to adopt calling conventions that are
not commonly used (if at all) in the language in question.

Worse, this protocol does not support any way to authorize users, encrypt
messages, or control which addresses they may use.

Therefore, this protocol will be replaced with a new implementation that
runs on the same port but fixes these problems.

This new protocol will be called ``KNXi`` ("KNX Integration").

Use cases
=========

``KNXi`` will be used in multiple scenarios:

* safe replacement for multicast links and tunnels (between ``knxd`` instances)

* applications that want to use KNX

* subscribing / querying the group cache

* support programming mode and other Layer 7 and management
  functions that had to be removed along with ``pthsem``

* any use case which requires authorization or encryption

Implementation
==============

``KNXi`` is based on `protobuf
<https://developers.google.com/protocol-buffers/>`_.

The protocol will also run on port 6720. The old and new protocols are
distinguished by the fact that the first byte of each message of the old
protocol is always zero, while our use of protobuf ensures that it is not.

The protobuf declaration is in ``src/knxi.proto``. All messages are of type ``KNXiMessage``.

Common fields
+++++++++++++

A ``KNXiMessage`` contains of two integers and a single ("oneof")
sub-message.

seq
---

Every packet contains a sequence number.

The system opening a connection shall use even sequence numbers.
Every sender's sequence numbers must be strictly monotonic. 

force_ack
---------

If set, the message's recipient shall reply with an ``ack`` message.
This reply shall be generated as close to the target as possible.

On replies, this flag shall be set if the message has been finally
delivered. Otherwise the reply signals that the message has been forwarded
to a broadcast or otherwise un-acknowledgable channel.

channel
-------

Some requests, e.g. starting to listen to a group address, will result in multiple messages. 
A channel is used to group these messages.

The message acknowledging the opening of a channel will assign a free channel.
Sender's and receiver's channel numbers are independent. 

Hello
+++++

The initial message shall be a HelloMessage. This document defines the "KNXi" protocol, version 1.0.

The server replies with a similar message. If there is no timely reply, the
client may retry using the old protocol.

This message shall not contain ``seq`` or ``channel`` fields.

Initial messages contain these keys:

* protocol

  Must be ``KNXi``.

* version

  The protocol version, as a sequence of integers. This document describes version 1.0.

* client

  A flag which the sender sets if it is a client. Servers do not set this
  flag.

* id

  A unique identifier for this client. A server shall assign the same
  address to this client as the last time it connected, if possible.
  No two clients with the same ID can be connected; in this case the older
  connection is terminated.

  IDs shall be ASCII strings. Servers should read their ID from their
  configuration; clients may generate a random UUID.

* agent

  The program name of the sender.

* agent-version

  The sender's program version, as a sequence of integers. Newer versions
  must have a higher (or longer) version number.

* addr

  The KNX address which the client wishes to use, or which the server
  assigns to it. When two servers connect, each transmits its own address.

  This is not ambiguous because a server never connects to a client, and
  the connecting system always transmits the first message.

* Flags

  A number of flags which reoprt available features or change behavior.

  This document declares the following flags:

  * has_groupcache

    The server shall support per-group caching of messages.

  * has_subscribe

    The server shall support subscribing to individual
    group addresses, or ranges thereof.

  * has_management

    The server shall support Layer7 or management features.

  * no_address

    Clients which do not require an address, e.g. because they only need to
    read the group cache, set this flag.

    A server which supports this flag will echo it, and transmit its own
    KNX address in the ``addr`` field.

  If a flag is unknown or not supported, the connection shall be terminated
  with a fatal error.

Options
+++++++

Reserved, currently empty.

Data
++++

A single KNX message. This is reserved for "write".

* src

  The sender of the message (2-byte string: physical address).

* dst

  The destination of the message (2-byte string: physical or group address)

  This field may be missing if the message appears on a channel that
  monitors exactly one group, or if it is destined for a client's assigned
  address.

* to_group

  A flag. If set, ``dst`` is a group address.

* data

  The message's bytes.

* short

  A flag. If set, the message shall be transmitted as a short message. In
  this case the data field must be one byte long and the value must be <
  ``0x3F``.

* cached

  The data in this message is from the server's cache.

  When set in a query, directs the server to answer from its cache if
  possible.

ack
+++

A message consisting of a single boolean value. If set, acknowledges the
``Data`` message sent with the same ``seq`` number. If cleared, sending was
unsiccessful, e.g. because the message was not acknowledged by the
recipient.

query
+++++

A query message, e.g. a "read" message.

This has the same fields as a ``Data`` message.

res
+++

A result message, e.g. the result of a "read" message.

This has the same fields as a ``Data`` message.

monitor
+++++++

Continually monitor ``data`` messages from this source / to this destination.

If ``cached`` is set, reply with an initial set of messages from the group cache.

The destination address may consist of four bytes; in that case the second
address is the (inclusive) end of the range to be monitored.

If both source and destination are given, both must match.

This message must be replied to with an ``ack``, that ``ack``'s channel
number is used to associate messages with the monitoring command.

err
+++

Signals an error to the partner. The error message is supposed to be human
readable. Parameters are error specific and will be interpolated into the
message.

close
+++++

Close a channel. 

Addressing
==========

All KNX addresses are transmitted as two-byte binary strings.

Some commands accept an address range; in that case the address is four
bytes long and the second half designates the last address in the range.

trace
-----

Request to receive log messages from the server.

This message is formatted like an error message; however, only
``severity``, ``subsys`` and ``code`` may be set (the latter only if
``subsys`` is present). The severity is a minimum requirement; other
parameters must match exactly.

Errors
======

Error replies (other than those forwarded by tracing) shall contain the
problem message's ``seq`` and ``channel``. If they are sent as a reaction
to a reply, using ``seq`` that is not possible (protocol violation); in
that case ``param2`` shall be used and ``param2_is_seq`` shall be set.

Spontaneous error messages shall use the normal rules for the sender's
``seq``.

Error subsys+code shall uniquely identify a single error message. The
following substitutions are used:

* {s}

  Seq of problem message

* {c}

  Channel of problem message.

* {1} to {4}

  Parameters 1 through 4, if set.


Error messages are described in ``src/errors.cpp``.

