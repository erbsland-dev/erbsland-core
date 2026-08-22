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
    result order = native resolver order after unsupported families and duplicates are removed

Source Lifecycle
----------------

.. code-block:: text

    source = event-loop-owned non-blocking network operation
    connection = active plaintext or authenticated application byte stream after protocol-specific setup
    inactive = configurable source without an active operation
    close = graceful terminal shutdown after accepted output drains
    abort or cancel = immediate terminal shutdown

Transport Behavior
------------------

.. code-block:: text

    TCP input = owned byte chunks without message boundaries
    TCP connection = one Connection source spanning outgoing establishment or accepted stream use
    UDP input = owned datagram preserving payload and remote endpoint
    send queue = finite atomic acceptance of one complete block or datagram
    would block = caller retains a rejected send until a writable transition is reported
    operational failure = failed source state is established before its error event
    TLS configuration = immutable application-wide entry selected by hierarchical label
    TLS fallback = exact label, then slash parents, then empty global default; never field-wise merging
    TLS client = one-shot Connection source active only after authenticated handshake completion
    common connection state = inactive, connecting or accepting, handshaking, active, closing, closed, or failed

HTTP Values
-----------

.. code-block:: text

    method = case-sensitive validated HTTP token with optional standard classification
    field name = ASCII-token identity compared and hashed with ASCII case folding
    field value = exact bytes excluding field-line edge OWS and prohibited controls; malformed UTF-8 and obs-text remain representable
    headers = ordered repeated fields with captured count and byte limits
    media type = canonical lowercase type, subtype, and parameter names with semantic parameter values
    request head = method, exact printable-ASCII request-target, version, and ordered headers
    response head = version, status, exact reason-phrase bytes, and ordered headers
    protocol scan = delimit raw octets before constructing String values
    field-line OWS = strip only leading and trailing SP/HTAB around the field value
    retained field bytes = preserve every interior byte without UTF-8 validation, normalization, or transcoding
    serialization = emit retained native String bytes with canonical name, colon, SP, and CRLF framing
    codec ownership = request/response state machines stay internal until the transaction API establishes their seam
    peer diagnostics = stable failure category plus nonsensitive local text; never include field values

Primary Types
=============

.. code-block:: text

    Network // event-loop frontend for asynchronous lookup and socket factories
    IpAddress, HostName, Host // resolved, Unicode-name, combined, and transport-format identities
    IpNetwork // normalized IPv4 or IPv6 CIDR network
    IpEndpoint, HostEndpoint // resolved and unresolved transport endpoints
    HostLookup // reusable asynchronous host resolution
    Connection, TcpConnection, TlsClientConnection, TlsServerConnection // common stream and concrete setup sources
    TcpListener // accepted TCP request source
    UdpSocket // reusable addressed UDP source bound once for one native socket lifetime

Secondary Types
===============

.. code-block:: text

    Port, ScopeId, IpVersion // endpoint components and address-family classification
    TcpConnectionRequest, TcpAcceptOptions // pending socket capability and accepted-stream configuration
    TcpConnectOptions, TcpListenerOptions // outgoing and listener configuration
    ConnectionQuota, ConnectionQuotaLease // shared admission capacity and move-only reservation
    ConnectionCloseContext, ConnectionCloseOrigin // common orderly stream termination details
    UdpDatagram, UdpDatagramDropContext // owned UDP payload and local drop details
    HostNameFormat // the host name format
    HostLookupEventEditor, TcpListenerEventEditor // source-owned lookup and listener handler editors
    ConnectionEventEditor, TcpConnectionEventEditor, UdpSocketEventEditor // common and concrete handler editors
    ConnectionState, NetworkSourceState, NetworkSendStatus // connection/source lifecycle and immediate send result
    HostLookupOptions // per-operation host lookup deadline and retry policy
    UdpSocketOptions // UDP payload and send-queue limits
    SocketBufferLimits // finite TCP send and receive queue limits
    NetworkError, NetworkErrorContext, NetworkErrorReason // throwable, structured, and machine-readable failures
    TlsClientConnectOptions // labeled setup, deadlines, and limits
    TlsServerAcceptOptions, TlsServerIdentityMapping // accepted TLS policy, default identity, and exact SNI entries
    TlsAlertDescription, NetworkErrorPhase // public TLS alert and failure phase metadata
    Url, UrlScheme // canonical absolute URL value and supported scheme classification
    UrlParseOptions, UrlFormatOptions // URL input limits and canonical output controls
    HttpMethod, HttpVersion, HttpStatus // method token, supported protocol version, and open status code
    HttpMediaType, HttpMediaTypeParameter // canonical media type and ordered unique parameter
    HttpHeaders, HttpField // bounded ordered header collection and one exact field
    HttpFieldName, HttpFieldType // case-insensitive name and recognized classification
    HttpHeaderLimits, HttpMethodType, HttpMethodTypes // header limits and standard-method flags
    HttpRequestHead, HttpResponseHead // reusable validated message control and header values
    HttpClientSession, HttpClientRequest, HttpClientResponse // client session, submitted operation, and retained reply
    HttpClientSessionOptions, HttpClientTlsOptions, HttpClientResponseOptions // client limits and response policy
    HttpClientRedirectOptions, HttpClientRedirectContext // captured redirect policy and one policy checkpoint
    HttpClientRedirectAction, HttpClientRedirectHostPolicy // redirect decisions and host boundary
    HttpClientRedirectConstraint // machine-readable non-bypassable redirect guard
    HttpCookieJar, HttpCookie, HttpCookieInsertionOptions, HttpCookieJarOptions // client cookie storage and snapshots
    HttpCookieSameSite, HttpClientBodyProgress // retained cookie metadata and committed download progress
    HttpServer, HttpServerRequest, HttpServerSession // server, retained transaction, and logical session sources
    HttpServerTlsOptions, HttpServerRouteOptions // curated HTTPS and automatic-route policy
    HttpServerSessionManager, HttpCookieSessionManager // synchronous and bounded cookie session selection
    HttpStaticContent, HttpStaticContentHandler // extensible static representation and source interfaces
    HttpStaticFileHandler, HttpStaticResourceHandler // abstract built-in-source interfaces with factories
    HttpMediaTypeMapping // immutable-or-copied suffix mapping shared by static handlers

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
    o.hasQuery()/hasFragment() -> bool // distinguish absent and explicitly empty delimiters
    o.toString([options]) -> String // create canonical transport text
    T::fromString(text[, options]) -> T // parse or return an invalid placeholder
    T::fromStringOrThrow(text[, options]) -> T // parse or throw ParseError
    o.resolved(reference[, options]) -> T // resolve a URI reference or return an invalid placeholder
    o.resolvedOrThrow(reference[, options]) -> T // resolve a URI reference or throw ParseError

HTTP Value Patterns
===================

.. code-block:: text

    T() // create an invalid placeholder or empty header collection
    T::fromString(text) -> T // parse a value or return an invalid placeholder
    T::fromStringOrThrow(text) -> T // parse a value or throw ParseError
    o.toString() -> String // create canonical text, except method and field-name spelling retained by their values
    o.standardType() -> optional<HttpMethodType> // classify an exact canonical standard method
    o.code()/defaultReasonPhrase() -> T // inspect numeric status and optional conventional phrase
    o.parameter(name)/setParameter(name, value) -> T // access or replace a media-type parameter
    o.hasField(name-or-type) -> bool // test whether at least one matching field exists
    o.getFirst/getAll(name-or-type) -> T // retrieve exact values in field order
    o.addField/setField/removeAllFields(...) -> T // mutate the bounded copy-on-write header collection
    o.method()/target()/version()/headers() -> T // inspect a request head
    o.version()/status()/reasonPhrase()/headers() -> T // inspect a response head

Source Creation Patterns
========================

.. code-block:: text

    o.createHostLookup() -> HostLookupPtr // create an inactive reusable lookup
    o.createTcpListener() -> TcpListenerPtr // create an inactive listener
    o.createTcpConnection() -> TcpConnectionPtr // create an inactive outgoing or accepted connection
    o.createTlsClientConnection() -> TlsClientConnectionPtr // create an inactive authenticated TLS client
    o.createTlsServerConnection() -> TlsServerConnectionPtr // create an inactive accepted TLS server source
    o.createUdpSocket() -> UdpSocketPtr // create an inactive addressed socket
    o.createHttpServer() -> HttpServerPtr // create an inactive session-first HTTP/1.1 server
    o.createHttpClientSession() -> HttpClientSessionPtr // create an active session-first HTTP/HTTPS client

Static Content Patterns
=======================

.. code-block:: text

    T::create(root-path[, url-prefix]) -> HttpStaticFileHandlerPtr // create one filesystem-tree handler
    T::create(resources, identifier[, url-prefix]) -> HttpStaticResourceHandlerPtr // use an injected provider
    T::create(identifier[, url-prefix]) -> HttpStaticResourceHandlerPtr // create an application-resource handler
    o.addStaticContentHandler(handler) // register an original polymorphic handler pointer while inactive
    o.hasPath(relative-path) -> bool // probe exact case-sensitive availability on a worker
    o.getContent(relative-path) -> HttpStaticContentPtr // create authoritative content after a positive probe
    o.length()/retainedMemoryLength() -> ByteLength // report exact output and retained-memory quota lengths
    o.open() -> ByteInputStreamPtr // one-shot open at byte zero; never return null
    o.urlPrefix()/setUrlPrefix(value) -> T // configure a complete-segment decoded prefix while unused
    o.setPriority(value) -> T& // select higher-first ordering with stable registration-order ties
    o.setIndexFileNames(names) -> T& // replace trailing-slash-only ordered index candidates while unused
    o.setMediaTypeMapping(mapping) -> T& // share a const mapping frozen during active server use
    T::create() -> HttpMediaTypeMappingPtr // create an empty mapping with application/octet-stream fallback
    T::defaultMapping() -> HttpMediaTypeMappingConstPtr // get the immutable built-in mapping
    o.copy() -> HttpMediaTypeMappingPtr // create an editable independent clone

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
    o.setAlpnProtocols(strings) // configure protocol identifiers as text::String values
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
    o.setAlpnProtocols(strings) // configure server-preference identifiers as text::String values
    o.onTransportConnected(callback) -> T& // TCP adoption completed, but the peer is unauthenticated
    o.onClientHello(callback) -> T& // inspect bounded SNI, ALPN, cipher, signature, and selected labels
    o.onHandshakeCompleted(callback) -> T& // client Finished is verified and application data becomes available
    o.send(bytes) -> NetworkSendStatus // atomically accept at most 16 KiB of application data
    o.pauseReceiving()/resumeReceiving() // propagate bounded receive flow through TLS and TCP
    o.close()/abort() // complete close_notify in both directions or stop immediately
    o.onClosed/onError/onFinal(callback) -> T& // observe exactly one terminal sequence

HTTP Server Patterns
====================

.. code-block:: text

    o.setOptions/setListenerOptions/setTcpAcceptOptions(...) // configure only while inactive
    o.enableTls([HttpServerTlsOptions]) // enable HTTPS with the http/server identity and forced HTTP/1.1 ALPN
    o.setSessionManager(manager) // replace anonymous per-connection selection with synchronous logical sessions
    o.start(local-endpoint) // bind after configuration and route registration
    o.events().onRequest(method, "/users/{id}", callback[, options]) // aggregate a bounded byte body
    o.events().onTextRequest(methods, pattern, callback[, options]) // aggregate and strictly decode UTF-8
    o.events().onJsonRequest(methods, pattern, callback[, options]) // aggregate and parse JSON
    o.events().onRequestHead(methods, pattern, callback) // select a low-level input policy manually
    o.streamBody/aggregateBody/rejectBody(...) // low-level policy at the paused head checkpoint
    o.sendText/sendJson/sendError/sendRedirect(...) // queue one request-bound fixed response
    o.startResponse/sendBody/finishBody(...) // only sendBody exposes atomic streamed back-pressure
    o.events().onBodyData(callback) // observe streamed body delivery; sibling events cover aggregation and trailers
    o.events().onWritable(callback) // retry only the rejected streamed body block
    o.invalidate() // prevent new selection and let the manager emit invalidation fields

HTTP Client Patterns
====================

.. code-block:: text

    o.createRequest([method,] absolute-url) -> HttpClientRequestPtr // create a prepared request
    o.sendRequest(request) // capture session defaults and submit one prepared request
    o.sendGet/sendHead/sendPost(...) -> HttpClientRequestPtr // create, submit, and retain a convenience request
    o.setDefaultHeaders(headers) // configure fields captured independently by each submission
    o.setRedirectOptions(options) // capture session or prepared-request redirect policy
    o.cookieJar() -> HttpCookieJar& // access stable session-owned cookie storage
    o.onResponse/onTextResponse/onJsonResponse(callback[, options]) // automatic bounded response handling
    o.onRedirect(callback) -> T& // inspect one redirect and return Follow, ReturnResponse, or Reject
    o.onResponseHead(callback) // select streaming, bounded aggregation, rejection, or a sink manually
    o.streamBody()/sendBody()/finishBody() // select and produce a chunked upload with atomic back-pressure
    o.aggregateBody/aggregateText/aggregateJson(limit) // select one low-level bounded response conversion
    o.writeBodyTo(output-or-path) // pump bounded chunks or atomically replace a path after success
    o.onBodyProgress(callback) // observe successfully committed sink bytes and optional expected length
    o.effectiveUrl()/redirectCount() // inspect the final URL and number of followed redirects
    o.cookies()/setCookie/removeCookie/clear(...) // inspect and mutate exact bounded cookie-jar state
    o.pauseBody()/resumeBody() // propagate streamed-download back-pressure
    o.close()/abort() // stop admission and drain, or cancel queued and active requests
