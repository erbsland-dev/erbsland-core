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
    host name = canonical lowercase NFC Unicode identity with a prevalidated strict IDNA2008 ASCII transport form
    host = numeric address or unresolved host name
    endpoint = host or address plus port and optional numeric IPv6 scope

Source Lifecycle
----------------

.. code-block:: text

    source = event-loop-owned non-blocking network operation
    inactive = configurable source without an active operation
    close = graceful terminal shutdown after accepted output drains
    abort or cancel = immediate terminal shutdown

Host Resolution
---------------

.. code-block:: text

    result order = native resolver order after unsupported families and duplicates are removed

Transport Behavior
------------------

.. code-block:: text

    TCP input = owned byte chunks without message boundaries
    TCP connection = one source spanning outgoing establishment or accepted stream use
    UDP input = owned datagram preserving payload and remote endpoint
    send queue = finite atomic acceptance of one complete block or datagram
    would block = caller retains a rejected send until a writable transition is reported
    operational failure = failed source state is established before its error event
    TLS configuration = immutable application-wide entry selected by hierarchical label
    TLS fallback = exact label, then slash parents, then empty global default; never field-wise merging
    TLS client = one-shot composed TCP source active only after authenticated handshake completion

Primary Types
=============

.. code-block:: text

    Network // event-loop frontend for asynchronous lookup and socket factories
    IpAddress, HostName, Host // resolved, Unicode-name, combined, and transport-format identities
    IpNetwork // normalized IPv4 or IPv6 CIDR network
    IpEndpoint, HostEndpoint // resolved and unresolved transport endpoints
    HostLookup // reusable asynchronous host resolution
    TcpListener, TcpConnection, TlsClientConnection, TlsServerConnection // transport and authenticated sources
    UdpSocket // reusable addressed UDP source bound once for one native socket lifetime

Secondary Types
===============

.. code-block:: text

    Port, ScopeId, IpVersion // endpoint components and address-family classification
    TcpConnectionRequest, TcpAcceptOptions // pending socket capability and accepted-stream configuration
    TcpConnectOptions, TcpListenerOptions // outgoing and listener configuration
    ConnectionQuota, ConnectionQuotaLease // shared admission capacity and move-only reservation
    TcpConnectionCloseContext, TcpConnectionCloseOrigin // normal stream termination details
    UdpDatagram, UdpDatagramDropContext // owned UDP payload and local drop details
    HostNameFormat // the host name format
    HostLookupEventEditor, TcpListenerEventEditor // source-owned lookup and listener handler editors
    TcpConnectionEventEditor, UdpSocketEventEditor // source-owned transport handler editors
    NetworkSourceState, NetworkSendStatus // source lifecycle and immediate send result
    HostLookupOptions // per-operation host lookup deadline and retry policy
    UdpSocketOptions // UDP payload and send-queue limits
    SocketBufferLimits // finite TCP send and receive queue limits
    NetworkError, NetworkErrorContext, NetworkErrorReason // throwable, structured, and machine-readable failures
    TlsClientConnectOptions, TlsClientConnectionState // labeled setup, deadlines, limits, and lifecycle
    TlsClientConnectionCloseContext, TlsClientConnectionCloseOrigin // orderly close-notify details
    TlsServerAcceptOptions, TlsServerIdentityMapping // accepted TLS policy, default identity, and exact SNI entries
    TlsServerConnectionState, TlsServerConnectionCloseContext, TlsServerConnectionCloseOrigin // server lifecycle
    TlsAlertDescription, NetworkErrorPhase // public TLS alert and failure phase metadata
    Url, UrlScheme // canonical absolute URL value and supported scheme classification
    UrlParseOptions, UrlFormatOptions // URL input limits and canonical output controls

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

URL Value Patterns
==================

.. code-block:: text

    T() // create an invalid placeholder
    T(endpoint[, path, query, fragment]) // create an HTTPS URL
    o.scheme()/schemeText()/isSecureScheme() -> T // inspect the URL scheme
    o.endpoint()/authorityText()/username()/password() -> T // inspect authority components
    o.path()/query()/fragment() -> String // inspect decoded NFC components
    o.toString([options]) -> String // create canonical transport text
    T::fromString(text[, options]) -> T // parse or return an invalid placeholder
    T::fromStringOrThrow(text[, options]) -> T // parse or throw ParseError

Source Creation Patterns
========================

.. code-block:: text

    o.createHostLookup() -> HostLookupPtr // create an inactive reusable lookup
    o.createTcpListener() -> TcpListenerPtr // create an inactive listener
    o.createTcpConnection() -> TcpConnectionPtr // create an inactive outgoing or accepted connection
    o.createTlsClientConnection() -> TlsClientConnectionPtr // create an inactive authenticated TLS client
    o.createTlsServerConnection() -> TlsServerConnectionPtr // create an inactive accepted TLS server source
    o.createUdpSocket() -> UdpSocketPtr // create an inactive addressed socket

Source Lifecycle Patterns
=========================

.. code-block:: text

    o.events() -> TEventEditor& // access the stable source-owned editor on the owner loop
    o.start(host[, options]) // start one reusable lookup operation
    o.start([local-endpoint][, options]) // bind one UDP socket lifetime
    o.state() -> NetworkSourceState // inspect source lifecycle
    o.pauseReceiving()/resumeReceiving() // suspend or resume bounded input delivery
    o.close()/abort() // request graceful or immediate terminal shutdown
    o.cancel() // cancel an active lookup
    o.on❮Event❯(callback) -> TEventEditor& // replace a source-owned handler through an editor
    o.onFinal(callback) -> TEventEditor& // observe normal, failed, or cancelled operation finalization

TCP Patterns
============

.. code-block:: text

    o.connect(endpoint[, options]) // resolve and connect within one overall deadline
    o.accept(request[, options]) // atomically consume a transferable request on the connection owner loop
    o.reject() // reject a pending connection request
    o.start(endpoint[, options]) // bind and activate a listener
    o.pauseAccepting()/resumeAccepting() // control native accept operations
    o.onHostResolved(callback) -> T& // validate the ordered candidates before any connection socket exists
    o.send(bytes) -> NetworkSendStatus // atomically accept or reject one byte block
    o.onData(callback) -> T& // receive owned chunks of the TCP byte stream
    o.onWritable(callback) -> T& // observe renewed capacity after back pressure
    o.onClosed/onError(callback) -> T& // observe ordered terminal outcomes
    o.tryAcquire([remote-endpoint]) -> optional<ConnectionQuotaLease> // reserve shared capacity atomically
    o.setConnectionQuota(quota) -> T& // share the accepted-connection limit across listeners

UDP Patterns
============

.. code-block:: text

    o.send(datagram-or-destination-and-bytes) -> NetworkSendStatus // atomically retain one addressed message
    o.localEndpoint() -> optional<IpEndpoint> // inspect the actual endpoint after binding
    o.onDatagram(callback) -> T& // receive addressed datagrams
    o.onDatagramDropped(callback) -> T& // inspect oversized incoming datagrams discarded locally
    o.onWritable(callback) -> T& // retry after a previous WouldBlock result
    o.onClosed/onError(callback) -> T& // observe ordered terminal outcomes

TLS Client Patterns
===================

.. code-block:: text

    o.connect(endpoint[, options]) // synchronously resolve labeled configuration, then start TCP
    o.configurationLabel() // options default to the reserved "tls/client" framework label
    o.onTransportConnected(callback) -> T& // TCP exists but TLS is not authenticated
    o.onPeerHello(callback) -> T& // inspect selected parameters without claiming peer identity
    o.onPeerAuthenticated(callback) -> T& // inspect the validated certificate path
    o.onHandshakeCompleted(callback) -> T& // authenticated application stream becomes active
    o.send(bytes) -> NetworkSendStatus // atomically accept at most 16 KiB of application data
    o.pauseReceiving()/resumeReceiving() // propagate bounded receive flow through TLS and TCP
    o.onClosed/onError/onFinal(callback) -> T& // observe exactly one terminal sequence

TLS Server Patterns
===================

.. code-block:: text

    o.accept(request, options) // resolve every immutable identity, acquire handshake capacity, then consume TCP
    o.configurationLabel() // options default to the reserved "tls/server" identity label
    o.setIdentityMappings(entries) // select at most 64 exact canonical SNI names; unmatched names use the default
    o.onTransportConnected(callback) -> T& // TCP adoption completed, but the peer is unauthenticated
    o.onClientHello(callback) -> T& // inspect bounded SNI, ALPN, cipher, signature, and selected labels
    o.onHandshakeCompleted(callback) -> T& // client Finished is verified and application data becomes available
    o.send(bytes) -> NetworkSendStatus // atomically accept at most 16 KiB of application data
    o.pauseReceiving()/resumeReceiving() // propagate bounded receive flow through TLS and TCP
    o.close()/abort() // complete close_notify in both directions or stop immediately
    o.onClosed/onError/onFinal(callback) -> T& // observe exactly one terminal sequence
