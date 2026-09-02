..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Network Connections; Reference
    single: Network Event Sources
    single: TCP
    single: UDP

*******************
Network Connections
*******************

Network Facade
==============

Availability
------------

``Network`` is accessed through ``Events::get<Network>()``.
The built-in backend resolves numeric addresses and host names asynchronously and provides native TCP and UDP sockets on
macOS, Linux, and Windows.
TCP uses kqueue, epoll, or IOCP through the event loop that owns each source.

See :doc:`/topics/network/resolving-hosts-asynchronously` for the complete host-lookup workflow, lifecycle,
cancellation, and diagnostics.

Buffering and Errors
--------------------

``SocketBufferLimits`` bounds accepted TCP output and buffered TCP input.
``ConnectionQuota`` bounds concurrent accepted resources with move-only leases.
Listener options create a private bounded quota by default, while an explicitly shared quota combines capacity across
listeners and event loops.
``UdpSocketOptions`` controls the maximum datagram size and accepted UDP output independently.
A send accepts or rejects a complete block or datagram immediately; ``Accepted`` means the source retained the complete
operation, while ``WouldBlock`` leaves ownership with the caller.
Operational failures carry ``NetworkErrorContext`` and a machine-readable ``NetworkErrorReason`` through source event
editors; invalid API use continues to throw synchronously.
TLS failures additionally report the ``NetworkErrorPhase`` and, when available, the exact public ``TlsAlertDescription``
wire value.
``NetworkError`` remains available when a handler wants to transfer a context to an exception-reporting boundary.

Accepted TLS
------------

``TlsServerAcceptOptions`` resolves a required default ``tls/server`` identity and up to 64 exact canonical SNI mappings
before it consumes a TCP request.
A required shared handshake quota independently bounds unauthenticated work and releases its lease when client Finished
is verified.
ALPN protocols are configured and reported as ``text::String`` values.
TLS preserves their exact raw bytes and only converts between string storage and protocol bytes at the wire boundary.
See :doc:`/topics/network/using-tls-server-connections` for identity registration, shared quotas, ClientHello policy,
back-pressure, and graceful closure.

Network Event Sources
=====================

Lifecycle and Affinity
----------------------

Every factory returns an inactive source.
Use a typed event editor on the owner loop to configure handlers before calling ``start()``.
The source owns one stable editor and all handlers configured through it.
Calling an ``on...()`` method again replaces that handler; an empty callback clears it.
Regular operations are owner-loop-only; cancellation and abort are thread-safe, and other cross-thread work uses
``Events::invoke()``.

``HostLookup`` is reusable: each ``start(host, options)`` call captures one host and one ``HostLookupOptions`` value.
Its result or error handler observes the terminal state, then ``onFinal()`` runs after the source returns to
``Inactive`` and can start another host immediately.

The built-in :cpp:class:`HostLookup <erbsland::network::HostLookup>` implementation is described in
:doc:`/topics/network/resolving-hosts-asynchronously`.
One-shot transport sources cannot be restarted after reaching ``Closed`` or ``Failed``.

Common Connection Interface
---------------------------

``Connection`` is the common application byte-stream interface implemented by ``TcpConnection``,
``TlsClientConnection``, and ``TlsServerConnection``.
Protocol-specific connection and handshake checkpoints remain on the concrete interfaces.
Once ``ConnectionState::Active`` is reached, generic consumers use the same endpoints, buffer limits, atomic send,
receive flow control, graceful close, abort, and common event handlers.

``ConnectionCloseContext`` identifies the first orderly-close initiator.
A remote TCP close is peer EOF, while a remote TLS close is an authenticated ``close_notify``.
Bare TCP EOF below TLS remains a truncation failure.

TLS Client Sources
------------------

``TlsClientConnection`` composes one ``TcpConnection`` and becomes ``Active`` only after server authentication and the
TLS 1.3 Finished exchange.
Its default configuration label is ``tls/client``.
The application configuration is resolved synchronously before TCP startup and retained as an immutable snapshot.

The ordered checkpoints are host resolution, transport connection, peer hello, peer authentication, and handshake
completion.
Each checkpoint automatically continues after its callback returns.
Calling ``abort()`` in a checkpoint prevents the next protocol transition or transport flight and emits only
``onFinal()``.

TLS Server Sources
------------------

``TlsServerConnection`` composes one accepted ``TcpConnection`` and remains in ``Accepting`` or ``Handshaking`` until
the client Finished authenticator is verified.
Before consuming the request, ``accept()`` validates all options, resolves the default and exact-SNI configuration
labels into immutable snapshots, and acquires a separate shared handshake-quota lease.
The ClientHello checkpoint exposes the bounded SNI and ALPN offer plus the selected cipher, signature scheme, and
configuration labels before secrets or the server flight are created.

Application flow, back-pressure, receive pause propagation, deadlines, truncation handling, ``close_notify``, and
terminal ordering match the client source.
See :doc:`/topics/network/using-tls-server-connections` for a complete listener example.

Application sends and data delivery occur only in ``Active``.
TLS records are transferred atomically to TCP, one TCP-rejected record is retained for writable retry, and receiving
pause propagates to the transport.
DNS/TCP, handshake, application idle, and graceful-close deadlines are independent.
Only accepted non-empty application sends and delivered non-empty application blocks move the idle deadline.

Orderly closure requires authenticated ``close_notify`` in both directions.
Bare TCP EOF is reported as TLS truncation.
Successful closure emits ``onClosed()`` followed by ``onFinal()``; failures emit ``onError()`` followed by
``onFinal()``.

TCP Sources
-----------

``TcpConnection`` spans both establishment and active stream use.
An outgoing connection resolves a named target, delivers the complete ordered endpoint list through
``onHostResolved()``, and tries endpoints in that order within one overall deadline.
Calling ``abort()`` in ``onHostResolved()`` prevents creation of a native connection socket.

``TcpListener`` applies its optional admission filter immediately after native accept.
The filter runs synchronously on the listener owner loop, before a ``TcpConnectionRequest`` is allocated or posted, and
therefore must be fast and non-blocking.
It limits sockets retained by the application but cannot prevent the operating system's SYN or listen backlog from
filling.

After the filter accepts a socket, the listener acquires its connection quota before allocating a request.
A full quota silently drops that socket and suspends native acceptance until capacity returns.
The move-only lease follows the socket through ``TcpConnectionRequest`` into ``TcpConnection`` and is released by
rejection, abandonment, failure, finalization, or destruction.
Sharing one quota pointer combines the limit across listeners and owner loops; default listener options instead create
one private quota.

Each emitted ``TcpConnectionRequest`` is a transferable, one-shot capability.
A prepared ``TcpConnection`` consumes it with ``accept(request, options)`` on the connection's destination loop.
Rejection is idempotent and thread-safe, and destroying an undecided request rejects it.
Closing the listener does not invalidate requests already emitted.

TCP data callbacks contain owned chunks of one byte stream and do not represent messages.
Graceful local closure drains accepted output; remote EOF delivers buffered input and drains accepted output.
``ConnectionCloseContext`` reports which side initiated the first normal close.
Aborting posts only ``onFinal()``, while normal close and failure post ``onFinal()`` after ``onClosed()`` or
``onError()``.

UDP Sources
-----------

``UdpSocket`` preserves datagram boundaries and remote addresses with ``UdpDatagram``.
It binds once, sends and receives any number of datagrams for multiple remote endpoints, and remains active until it is
closed or aborted.
``UdpSocketOptions`` independently bounds datagram size and accepted output.
See :doc:`/topics/network/using-udp-sockets` for binding, back-pressure, drops, and shutdown.

Interface
=========

.. doxygenclass:: erbsland::network::HostLookup
    :members:

.. doxygentypedef:: erbsland::network::HostLookupPtr
.. doxygenclass:: erbsland::network::HostLookupEventEditor
    :members:
.. doxygenclass:: erbsland::network::HostLookupOptions
    :members:
.. doxygentypedef:: erbsland::network::HostResolvedFn
.. doxygenclass:: erbsland::network::Network
    :members:
.. doxygenclass:: erbsland::network::Connection
    :members:

.. doxygentypedef:: erbsland::network::ConnectionPtr
.. doxygenclass:: erbsland::network::ConnectionCloseContext
    :members:
.. doxygentypedef:: erbsland::network::ConnectionCloseFn
.. doxygenenum:: erbsland::network::ConnectionCloseOrigin
.. doxygenclass:: erbsland::network::ConnectionEventEditor
    :members:
.. doxygenclass:: erbsland::network::ConnectionQuota
    :members:
.. doxygenclass:: erbsland::network::ConnectionQuotaLease
    :members:
.. doxygenenum:: erbsland::network::ConnectionState
.. doxygentypedef:: erbsland::network::NetworkDataFn
.. doxygenclass:: erbsland::network::NetworkError
    :members:
.. doxygenclass:: erbsland::network::NetworkErrorContext
    :members:
.. doxygentypedef:: erbsland::network::NetworkErrorFn
.. doxygenenum:: erbsland::network::NetworkErrorPhase
.. doxygenenum:: erbsland::network::NetworkErrorReason
.. doxygentypedef:: erbsland::network::NetworkEventFn
.. doxygenclass:: erbsland::network::NetworkSendStatus
    :members:
.. doxygenenum:: erbsland::network::NetworkSourceState
.. doxygenclass:: erbsland::network::SocketBufferLimits
    :members:
.. doxygenclass:: erbsland::network::TcpAcceptOptions
    :members:
.. doxygenclass:: erbsland::network::TcpConnection
    :members:

.. doxygentypedef:: erbsland::network::TcpConnectionPtr
.. doxygenclass:: erbsland::network::TcpConnectionEventEditor
    :members:
.. doxygentypedef:: erbsland::network::TcpConnectionFilterFn
.. doxygenenum:: erbsland::network::TcpConnectionFilterResult
.. doxygentypedef:: erbsland::network::TcpConnectionFn
.. doxygenclass:: erbsland::network::TcpConnectionRequest
    :members:

.. doxygentypedef:: erbsland::network::TcpConnectionRequestPtr
.. doxygentypedef:: erbsland::network::TcpConnectionRequestFn
.. doxygenenum:: erbsland::network::TcpConnectionRequestState
.. doxygenclass:: erbsland::network::TcpConnectOptions
    :members:
.. doxygentypedef:: erbsland::network::TcpHostResolvedFn
.. doxygenclass:: erbsland::network::TcpListener
    :members:

.. doxygentypedef:: erbsland::network::TcpListenerPtr
.. doxygenclass:: erbsland::network::TcpListenerEventEditor
    :members:
.. doxygenclass:: erbsland::network::TcpListenerOptions
    :members:
.. doxygenenum:: erbsland::network::TlsAlertDescription
.. doxygenclass:: erbsland::network::TlsClientConnection
    :members:
.. doxygenclass:: erbsland::network::TlsClientConnectionEventEditor
    :members:
.. doxygenclass:: erbsland::network::TlsClientConnectOptions
    :members:
.. doxygenclass:: erbsland::network::TlsServerAcceptOptions
    :members:
.. doxygenclass:: erbsland::network::TlsServerConnection
    :members:
.. doxygenclass:: erbsland::network::TlsServerConnectionEventEditor
    :members:
.. doxygenclass:: erbsland::network::TlsServerIdentityMapping
    :members:
.. doxygenclass:: erbsland::network::UdpDatagram
    :members:
.. doxygenclass:: erbsland::network::UdpDatagramDropContext
    :members:
.. doxygentypedef:: erbsland::network::UdpDatagramDropFn
.. doxygenenum:: erbsland::network::UdpDatagramDropReason
.. doxygentypedef:: erbsland::network::UdpDatagramFn
.. doxygenclass:: erbsland::network::UdpSocket
    :members:

.. doxygentypedef:: erbsland::network::UdpSocketPtr
.. doxygenclass:: erbsland::network::UdpSocketEventEditor
    :members:
.. doxygenclass:: erbsland::network::UdpSocketOptions
    :members:
