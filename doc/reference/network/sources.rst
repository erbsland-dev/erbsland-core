.. index::
    single: Network Event Sources
    single: TCP
    single: UDP

*********************
Network Event Sources
*********************

Lifecycle and Affinity
======================

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

TLS Client Sources
==================

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
==================

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
===========

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
``TcpConnectionCloseContext`` reports which side initiated the first normal close.
Aborting posts only ``onFinal()``, while normal close and failure post ``onFinal()`` after ``onClosed()`` or
``onError()``.

UDP Sources
===========

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
.. doxygenclass:: erbsland::network::TcpConnection
    :members:

.. doxygentypedef:: erbsland::network::TcpConnectionPtr
.. doxygenclass:: erbsland::network::TcpConnectionEventEditor
    :members:
.. doxygenclass:: erbsland::network::TcpConnectionRequest
    :members:

.. doxygentypedef:: erbsland::network::TcpConnectionRequestPtr
.. doxygenclass:: erbsland::network::TcpListener
    :members:

.. doxygentypedef:: erbsland::network::TcpListenerPtr
.. doxygenclass:: erbsland::network::TcpListenerEventEditor
    :members:
.. doxygenclass:: erbsland::network::UdpSocket
    :members:

.. doxygentypedef:: erbsland::network::UdpSocketPtr
.. doxygenclass:: erbsland::network::UdpSocketEventEditor
    :members:
.. doxygenclass:: erbsland::network::UdpSocketOptions
    :members:
