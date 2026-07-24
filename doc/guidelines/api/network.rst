*****************************
Network Domain API Guidelines
*****************************

Core Semantics
==============

Addressing
----------

.. code-block:: text

    address = numeric IPv4 or IPv6 identity without port or scope
    network = normalized CIDR address range
    host name = opaque name passed to the platform resolver
    host = numeric address or unresolved host name
    endpoint = host or address plus port and optional numeric IPv6 scope

Source Lifecycle
----------------

.. code-block:: text

    source = event-loop-owned non-blocking network operation
    inactive = configurable source unable to emit completion events before start
    subscription = retained callbacks removed together on release or disconnection
    close = graceful terminal shutdown after accepted output drains
    abort or cancel = immediate terminal shutdown
    regular operation = owner-loop only; cancellation and abort are thread-safe

Transport Behavior
------------------

.. code-block:: text

    TCP input = owned byte chunks without message boundaries
    TCP request = pending accept-or-reject decision; abandoned request is rejected
    UDP input = owned datagram preserving payload and remote endpoint
    send queue = finite atomic acceptance of one complete block or datagram
    back pressure = rejected send until a writable transition is reported
    operational failure = ordered error event followed by failed source state

Primary Types
=============

.. code-block:: text

    Network // event-loop frontend for asynchronous lookup and socket factories
    IpAddress, HostName, Host // resolved, unresolved, and combined host identities
    IpNetwork // normalized IPv4 or IPv6 CIDR network
    IpEndpoint, HostEndpoint // resolved and unresolved transport endpoints
    HostLookup // one-shot asynchronous host resolution
    TcpListener, TcpConnection // listening and connected TCP sources
    UdpSocket, UdpPeer // addressed and fixed-peer UDP sources

Secondary Types
===============

.. code-block:: text

    Port, ScopeId, IpVersion // endpoint components and address-family classification
    TcpConnectionRequest // pending TCP accept or reject decision
    TcpConnectionAttempt // outgoing resolution and connection sequence
    UdpDatagram // owned UDP payload and remote endpoint
    HostLookupEvents, TcpListenerEvents, TcpConnectionAttemptEvents // retained operation subscriptions
    TcpConnectionEvents, UdpSocketEvents, UdpPeerEvents // retained transport subscriptions
    NetworkSourceState, NetworkSendStatus // source lifecycle and immediate send result
    SocketBufferLimits // finite send and receive queue limits
    NetworkError, NetworkErrorContext // operational failure and structured context

Address Value Patterns
======================

.. code-block:: text

    T::fromString(text) -> T // parse with an invalid or empty fallback
    T::fromStringOrThrow(text) -> T // parse or throw err::ParseError
    o.toString() -> text::String // create canonical text
    o.hash() -> std::size_t // hash consistently with equality
    o.ipVersion() -> IpVersion // inspect the address family
    o.contains(address-or-network) -> bool // test CIDR containment
    o.firstAddress()/lastAddress() -> IpAddress // get inclusive network boundaries

Source Creation Patterns
========================

.. code-block:: text

    o.createHostLookup(host) -> HostLookupPtr // create an inactive lookup
    o.createTcpListener(endpoint[, backlog]) -> TcpListenerPtr // create an inactive listener
    o.createTcpConnection(endpoint[, limits]) -> TcpConnectionAttemptPtr // create an inactive outgoing attempt
    o.createUdpSocket(endpoint[, limits]) -> UdpSocketPtr // create an inactive addressed socket
    o.createUdpPeer(endpoint[, limits]) -> UdpPeerPtr // create an inactive fixed-peer source

Source Lifecycle Patterns
=========================

.. code-block:: text

    o.events() -> TEventsPtr // create a retained callback subscription on the owner loop
    o.start() // activate a fully configured inactive source
    o.state() -> NetworkSourceState // inspect source lifecycle
    o.pauseReceiving()/resumeReceiving() // suspend or resume bounded input delivery
    o.close()/abort() // request graceful or immediate terminal shutdown
    o.cancel() // cancel a one-shot lookup or connection attempt
    o.on❮Event❯(callback) -> TEvents& // add a callback through a retained editor

TCP Patterns
============

.. code-block:: text

    o.accept(events, setup) // accept a request and configure an inactive connection on its owner loop
    o.reject() // reject a pending connection request
    o.send(bytes) -> NetworkSendStatus // atomically accept or reject one byte block
    o.onData(callback) -> T& // receive owned chunks of the TCP byte stream
    o.onWritable(callback) -> T& // observe renewed capacity after back pressure
    o.onClosed/onError(callback) -> T& // observe ordered terminal outcomes

UDP Patterns
============

.. code-block:: text

    o.send(datagram-or-bytes) -> NetworkSendStatus // atomically accept one addressed or fixed-peer message
    o.onDatagram/onData(callback) -> T& // receive addressed datagrams or fixed-peer payloads
    o.onWritable(callback) -> T& // observe renewed capacity after back pressure
    o.onClosed/onError(callback) -> T& // observe ordered terminal outcomes
