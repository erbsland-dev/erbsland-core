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
Create and retain one or more typed event editors on its owner loop before calling ``start()``.
The source holds editors weakly, so releasing or disconnecting an editor removes its subscription.
Regular operations are owner-loop-only; cancellation and abort are thread-safe, and other cross-thread work uses
``Events::invoke()``.

TCP Sources
===========

``TcpListener`` emits ``TcpConnectionRequest`` objects.
Accepting selects a target event loop and invokes a setup callback there with an inactive ``TcpConnection``.
Outgoing ``TcpConnectionAttempt`` objects resolve names internally and preserve resolver order.
TCP data callbacks contain owned chunks of one byte stream and do not represent messages.

UDP Sources
===========

``UdpSocket`` preserves datagram boundaries and remote addresses with ``UdpDatagram``.
``UdpPeer`` resolves one remote host and then exchanges payload blocks with that fixed endpoint.

Interface
=========

.. doxygenclass:: erbsland::network::HostLookup
    :members:
.. doxygenclass:: erbsland::network::HostLookupEvents
    :members:
.. doxygenclass:: erbsland::network::TcpConnection
    :members:
.. doxygenclass:: erbsland::network::TcpConnectionAttempt
    :members:
.. doxygenclass:: erbsland::network::TcpConnectionAttemptEvents
    :members:
.. doxygenclass:: erbsland::network::TcpConnectionEvents
    :members:
.. doxygenclass:: erbsland::network::TcpConnectionRequest
    :members:
.. doxygenclass:: erbsland::network::TcpListener
    :members:
.. doxygenclass:: erbsland::network::TcpListenerEvents
    :members:
.. doxygenclass:: erbsland::network::UdpPeer
    :members:
.. doxygenclass:: erbsland::network::UdpPeerEvents
    :members:
.. doxygenclass:: erbsland::network::UdpSocket
    :members:
.. doxygenclass:: erbsland::network::UdpSocketEvents
    :members:
