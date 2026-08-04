**************
Network Facade
**************

Availability
============

``Network`` is accessed through ``Events::get<Network>()``.
The built-in backend resolves numeric addresses and host names asynchronously and provides native TCP and UDP sockets on
macOS, Linux, and Windows.
TCP uses kqueue, epoll, or IOCP through the event loop that owns each source.

See :doc:`/topics/network/resolving-hosts-asynchronously` for the complete host-lookup workflow, lifecycle,
cancellation, and diagnostics.

Buffering and Errors
====================

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
============

``TlsServerAcceptOptions`` resolves a required default ``tls/server`` identity and up to 64 exact canonical SNI mappings
before it consumes a TCP request.
A required shared handshake quota independently bounds unauthenticated work and releases its lease when client Finished
is verified.
See :doc:`/topics/network/using-tls-server-connections` for identity registration, shared quotas, ClientHello policy,
back-pressure, and graceful closure.

Interface
=========

.. doxygentypedef:: erbsland::network::HostResolvedFn
.. doxygenclass:: erbsland::network::Network
    :members:
.. doxygenclass:: erbsland::network::ConnectionQuota
    :members:
.. doxygenclass:: erbsland::network::ConnectionQuotaLease
    :members:
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
.. doxygenclass:: erbsland::network::TcpConnectionCloseContext
    :members:
.. doxygentypedef:: erbsland::network::TcpConnectionCloseFn
.. doxygenenum:: erbsland::network::TcpConnectionCloseOrigin
.. doxygentypedef:: erbsland::network::TcpConnectionFilterFn
.. doxygenenum:: erbsland::network::TcpConnectionFilterResult
.. doxygentypedef:: erbsland::network::TcpConnectionFn
.. doxygentypedef:: erbsland::network::TcpConnectionRequestFn
.. doxygenenum:: erbsland::network::TcpConnectionRequestState
.. doxygenclass:: erbsland::network::TcpConnectOptions
    :members:
.. doxygentypedef:: erbsland::network::TcpHostResolvedFn
.. doxygenclass:: erbsland::network::TcpListenerOptions
    :members:
.. doxygenenum:: erbsland::network::TlsAlertDescription
.. doxygenclass:: erbsland::network::TlsClientConnection
    :members:
.. doxygenclass:: erbsland::network::TlsClientConnectionCloseContext
    :members:
.. doxygentypedef:: erbsland::network::TlsClientConnectionCloseFn
.. doxygenenum:: erbsland::network::TlsClientConnectionCloseOrigin
.. doxygenclass:: erbsland::network::TlsClientConnectionEventEditor
    :members:
.. doxygenenum:: erbsland::network::TlsClientConnectionState
.. doxygenclass:: erbsland::network::TlsClientConnectOptions
    :members:
.. doxygenclass:: erbsland::network::TlsServerAcceptOptions
    :members:
.. doxygenclass:: erbsland::network::TlsServerConnection
    :members:
.. doxygenclass:: erbsland::network::TlsServerConnectionCloseContext
    :members:
.. doxygentypedef:: erbsland::network::TlsServerConnectionCloseFn
.. doxygenenum:: erbsland::network::TlsServerConnectionCloseOrigin
.. doxygenclass:: erbsland::network::TlsServerConnectionEventEditor
    :members:
.. doxygenenum:: erbsland::network::TlsServerConnectionState
.. doxygenclass:: erbsland::network::TlsServerIdentityMapping
    :members:
.. doxygenclass:: erbsland::network::UdpDatagram
    :members:
.. doxygenclass:: erbsland::network::UdpDatagramDropContext
    :members:
.. doxygentypedef:: erbsland::network::UdpDatagramDropFn
.. doxygenenum:: erbsland::network::UdpDatagramDropReason
.. doxygentypedef:: erbsland::network::UdpDatagramFn
