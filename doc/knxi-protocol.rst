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

* fix support for programming mode and other Layer 7 and management
  functions that had to be removed along with ``pthsem``

* any use case which requires authorization or encryption

Implementation
==============

``KNXi`` is based on `msgpack
<https://github.com/msgpack/msgpack/blob/master/spec.md>`_, a lean protocol
that is easy to analyze and extend (if necessary). For message compactness,
keys are shortened to their initial letter (or letters, if they contain a
hyphen) unless indicated otherwise.

The initial message will look like this::

    ["KNXi", {'v':[0,1]} ]

This translates to a binary string which always starts with::

    92 A4 4B 4E 58 69 …

which is illegal in the context of the legacy protocol.

The server replies with a similar message.

Initial messages contain these keys:

* version

  The protocol version. This document describes version 0.1. Version
  numbers are sequences of integers.

* role

  Role (c/s (client or server)). By convention, the node initiating the TCP
  connection is the client, unless indicated otherwise.

* id

  A unique identifier for this client. A server shall assign the same
  address to this client as the last time it connected, if possible.
  No two clients with the same ID can be connected; in this case the older
  connection is terminated.

  IDs shall be ASCII strings. Servers should read their ID from their
  configuration; clients may generate a random UUID.

* agent-name

  The program name of the sender.

* agent-version

  The sender's program version, as a sequence of integers. Newer versions
  must have a higher (or longer) version number.

* address

  The KNX address which the client wishes to use, or which the server
  assigns to it. When two servers connect, each transmits its own address.

* phys-addrs

  Physical addresses which the server sending this message knows to be
  located on some of its ports. This is a list of addresses or
  ``[start,end]`` address ranges.

  XXX: TODO: Should registration of known addresses be fatal?

* features

  A map of features which the sending node wishes the recipient to
  support.

  This document declares the following features:

  * groupcache

    The server shall support per-group caching of messages.

  * subscription

    The server shall support subscribing to individual group addresses, or
    ranges thereof.

  * address assignment

    Clients which do not require an address, e.g. because they only need to
    read the group cache, 

  * management

    The server shall support management features.

If a feature is not supported, the recipient shall reply with error
message 101 (the missing feature shall be the first parameter) and close
the connection.

All other messages shall consist of a mapping that may contain the
following keys:

* action

  The presence of this key indicates a request.

  Most ``action`` message must contain a new, unique sequence number. See below.

  Some actions may consist of multiple messages. In this case the first
  message must contain a ``state:begin`` element; the last ``state:end``.
  A one-element sequence shall contain none of these. A zero-element
  message is simply missing.

  The same rule applies to replies.

  See next section for possible values of this field.

* seqnum

  A sequence number.

  If both partners initially transmitted IDs, the node which sent the
  lexically-larger ID shall use even sequence numbers; otherwise the client
  shall do so.

  Sequence numbers should be consecutive ``uint`` values. They should not
  be reused. If they are, sequence numbers that are still in use must be
  skipped.

  Most actions require sequence numbers, in order to associate replies with
  the request that generated it.

* state

  This field may contain the values ``b`` (begin) or ``e`` (end).
  A ``begin`` value starts a multi-reply sequence, ``end`` terminates it.
  A node must not send more messages with this seqnum after sending an
  ``end``.

  TODO: Use states ``pause`` and ``continue`` for flow control?

* error

  Exception value. This element shall contain a list, containing

  * a severity number (0=trace 1=debug 2=note 3=info 4=warning 5=error
    6=fatal 7=abort). Errors >=5, transmitted without a sequence number,
    will close the connection.

  * an error number, which may be used to find the corresponding code on
    the server

  * a human-readable error message

  * parameter(s) required to interpret the error message.

  An error message without a sequence number terminates a connection.

* message

  KNX data to be transmitted. Unless otherwise specified, this element
  shall contain the byte string (msgpack: ``bin``) to be transmitted.
  
  A one-byte message that shall be transmitted as a short KNX message must
  be encoded as a ``uint8`` value; unuseable bits must be clear.

* source

  Physical source address. If missing, the server uses the address
  assigned to the connection.

* dest

  Destination address.

* timeout

  An integer (milliseconds) indicating how long to wait for an answer.
  The default is zero.

Addressing
==========

All KNX addresses are transmitted as two-byte extension (``fixext 2``
according to the msgpack spec). Physical addresses shall use type code 1,
group addresses use type code 2.

Actions
=======

A node which does not implement an action must reply with an error
message. Some basic actions, most notably immediate packet data, may be
transmitted without a seqnum if explicitly allowed.

stop
----

This is the only action that uses an existing sequence number; it
directs the stream with that seqnum to terminate.

The opposing node nust acknowledge the termination with a ``state:end``
message.

If sent without a seqnum, this message indictes an intent to terminate the
connection cleanly (as opposed to simply closing it). The recipient should
close all data streams and then reply with a similar message.

write
-----

Send the ``message`` to the ``dest`` or ``group``.

If the message is stored as a ``uint8`` number, the message is a short
write; bits that do not fit in a short message must be clear.

This message may be sent without a seqnum.

read
----

This message contains an unsolicited ``read`` message.

This message may be sent without a seqnum.

query
-----

Request to read the value at the given (group) address.

The destination address field may contain a list of addresses or ``[begin,end]``
pairs, which queries all of the given addresses or address ranges.

If the timeout is >0, knxd will wait for the answer and return it to the
client. Otherwise it will acknowledge having transmitted the query; the
answer will (or will not) arrive in a separate ``read`` message if the
client subscribed to the group.

If the timeout is negative, the group cache will be consulted. If the
value is not cached, a timeout of -1 results in an error; otherwise the
timeout is treated as if its value was ``(2-timeout)``.

A missing destination field is regarded as encompassing the complete
address range. However, to avoid flooding the bus erroneously, it can only
be used with a timeout of zero.

This message may be sent without a seqnum if a single destination address
and no timeout are given.

subscribe
---------

As ``query``, but also transmit new messages directed at the group(s) in
question.

This action runs until terminated. A seqnum is required.

A simple monitoring client would thus send::

    { 'a':'s', 's':1 }

monitor
-------

Request to receive messages involving a given physical address.

If no address is given, the server's complete traffic is monitored.

trace
-----

Request to receive log messages from the server.

The ``filter`` element shall contain the trace level, or a two-integer
array with the trace level and a bitmask (server subsystems to trace).

TODO: replace the second element with a server-specific list of subsystem names.


Errors
======

Error messages shall be transmitted in a message containing a valid
sequence number. If that is not possible, either because the error has not
been triggered by a request or because the initial message was invalid, the
error's seqnum shall be missing and the protocol connection shall be
terminated after sending the error message.

The following error numbers are defined by this protocol. Any requisite
contents of the error message's parameter list is given in parentheses.

* 0

  Reserved.

* 1

  Data format error

* 2-99

  Reserved for protocol errors.

* 100

  Your address is already allocated. (address)

* 101

  Unsupported feature. (feature name)

* 102

  Sequence number is in use. (seqnum)

* 103

  Sequence number is odd/even. (seqnum)

* 104-999

  Reserved.

* 1000-9999

  server-specific.

* 10000…19999

  Client-specific.

* 20000…

  Reserved.

