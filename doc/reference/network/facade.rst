.. index::
    single: Network Facade
    single: Network Errors

**************
Network Facade
**************

Availability
============

``Network`` is accessed through ``Events::get<Network>()``.
This iteration registers the built-in backend identifier and provides the complete abstract facade, but deliberately
does not install a default DNS or socket backend.
Production lookup and socket requests therefore report the frontend as unavailable until a native network backend is
added.
Tests and applications can register a mock or custom backend explicitly.

Buffering and Errors
====================

``SocketBufferLimits`` bounds accepted input and output, with a default of one mebibyte in each direction.
A send accepts or rejects a complete block or datagram immediately.
Operational failures carry ``NetworkErrorContext`` and arrive through source event editors; invalid API use continues to
throw synchronously.

Interface
=========

.. doxygenclass:: erbsland::network::Network
    :members:

.. doxygentypedef:: erbsland::network::HostLookupPtr

.. doxygentypedef:: erbsland::network::HostLookupEventsPtr

.. doxygentypedef:: erbsland::network::TcpConnectionPtr

.. doxygentypedef:: erbsland::network::TcpConnectionAttemptPtr

.. doxygentypedef:: erbsland::network::TcpConnectionAttemptEventsPtr

.. doxygentypedef:: erbsland::network::TcpConnectionEventsPtr

.. doxygentypedef:: erbsland::network::TcpConnectionRequestPtr

.. doxygentypedef:: erbsland::network::TcpListenerPtr

.. doxygentypedef:: erbsland::network::TcpListenerEventsPtr

.. doxygentypedef:: erbsland::network::UdpPeerPtr

.. doxygentypedef:: erbsland::network::UdpPeerEventsPtr

.. doxygentypedef:: erbsland::network::UdpSocketPtr

.. doxygentypedef:: erbsland::network::UdpSocketEventsPtr
.. doxygentypedef:: erbsland::network::NetworkEventFn

.. doxygentypedef:: erbsland::network::NetworkErrorFn

.. doxygentypedef:: erbsland::network::HostResolvedFn

.. doxygentypedef:: erbsland::network::TcpConnectionRequestFn

.. doxygentypedef:: erbsland::network::TcpConnectionFn

.. doxygentypedef:: erbsland::network::NetworkDataFn

.. doxygentypedef:: erbsland::network::UdpDatagramFn
.. doxygenclass:: erbsland::network::NetworkError
    :members:
.. doxygenclass:: erbsland::network::NetworkErrorContext
    :members:
.. doxygenclass:: erbsland::network::NetworkSendStatus
    :members:
.. doxygenenum:: erbsland::network::NetworkSourceState
.. doxygenclass:: erbsland::network::SocketBufferLimits
    :members:
.. doxygenclass:: erbsland::network::UdpDatagram
    :members:
