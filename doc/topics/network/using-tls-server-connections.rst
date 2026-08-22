.. index::
    single: TLS Server Connection
    single: Connection Quota
    single: SNI Identity Selection

****************************
Using TLS Server Connections
****************************

An accepted TLS server starts with a transferable TCP request and becomes an application byte stream only after it has
authenticated client Finished.
This page shows how to share connection capacity across listeners, limit incomplete handshakes separately, select
immutable identities by exact SNI, inspect ClientHello policy, and shut down without truncating TLS.

Registering Default and Exact Identities
========================================

Load protected server identities once during application startup and register complete TLS configurations under labels.
The default label is required; exact SNI mappings may select other labels:

.. code-block:: cpp

    auto defaultTls = cryptology::TlsConfiguration{};
    defaultTls.setServerIdentity(loadIdentity("default-chain.pem"_el, "default-key.pem"_el));
    core::application().cryptologyConfiguration().setTlsConfiguration(
        "tls/server"_el, std::move(defaultTls));

    auto apiTls = cryptology::TlsConfiguration{};
    apiTls.setServerIdentity(loadIdentity("api-chain.pem"_el, "api-key.pem"_el));
    core::application().cryptologyConfiguration().setTlsConfiguration(
        "tls/server/api"_el, std::move(apiTls));

``accept()`` resolves and captures every configured label before consuming the TCP request.
Replacing a registry entry therefore affects later accepts without changing an in-progress handshake.
SNI names use canonical ``HostName`` equality, so Unicode and IDNA ASCII spellings select the same exact entry.
An absent or unmatched SNI selects the required default identity.
An exact identity that cannot use any offered signature scheme fails the handshake instead of falling back.

Sharing Capacity Across Listeners
=================================

TCP capacity covers every accepted socket from request creation through connection finalization.
Pass the same quota pointer to multiple listeners to combine their limit.
Use a different shared quota for incomplete TLS handshakes so authenticated connections no longer consume scarce
handshake capacity:

.. code-block:: cpp

    auto tcpQuota = network::ConnectionQuota::create(unit::ItemCount{1000U});
    auto handshakeQuota = network::ConnectionQuota::create(unit::ItemCount{64U});

    auto publicListener = events->get<network::Network>().createTcpListener();
    auto adminListener = events->get<network::Network>().createTcpListener();

    publicListener->start(
        publicEndpoint,
        network::TcpListenerOptions{}.setConnectionQuota(tcpQuota));
    adminListener->start(
        adminEndpoint,
        network::TcpListenerOptions{}.setConnectionQuota(tcpQuota));

A full TCP quota rejects a newly accepted socket without emitting a request and suspends native acceptance until a lease
is released.
Rejected, abandoned, failed, and closed connections all release exactly once.
Listeners without an explicit quota receive separate bounded defaults.

Accepting TLS and Inspecting ClientHello
========================================

Create one TLS source for each emitted TCP request.
The ClientHello checkpoint runs after bounded parsing and exact identity, ALPN, cipher, and signature selection, but
before the server creates secrets or emits its flight:

.. code-block:: cpp

    publicListener->events().onConnection(
        [events, handshakeQuota](network::TcpConnectionRequestPtr request) {
            auto connection = events->get<network::Network>().createTlsServerConnection();
            connection->events()
                .onTransportConnected([]() {
                    // TCP is connected; TLS is not authenticated.
                })
                .onClientHello([connection]() {
                    inspect(
                        connection->serverName(),
                        connection->offeredAlpn(),
                        connection->requestedConfigurationLabel(),
                        connection->matchedConfigurationLabel());
                })
                .onHandshakeCompleted([connection]() {
                    beginApplicationProtocol(connection);
                });

            auto options = network::TlsServerAcceptOptions{handshakeQuota};
            options.setIdentityMappings({
                {network::HostName::fromStringOrThrow("api.example.test"_el), "tls/server/api"_el}})
                .setAlpnProtocols({"http/1.1"_el});
            connection->accept(std::move(request), std::move(options));
        });

Calling ``abort()`` in ``onTransportConnected`` or ``onClientHello`` prevents the next transition and emits no server
flight.
A full handshake quota rejects the request and reports ``ResourceLimitExceeded`` in the ``Accepting`` phase on the TLS
source.

Back-Pressure and Graceful Shutdown
===================================

Application sends begin only in ``Active`` and accept at most 16 KiB atomically.
Keep a rejected block and retry it from ``onWritable``.
``pauseReceiving()`` stops application delivery and propagates the pause through TLS into TCP while all retained bytes
remain within the configured TLS and transport limits.

``close()`` sends ``close_notify`` and waits for the peer notification before closing TCP.
If the peer starts closure, the source replies before finalizing.
A bare TCP EOF is reported as TLS truncation.
Successful closure calls ``onClosed`` with the first closure origin and then calls ``onFinal`` exactly once; failure
calls ``onError`` followed by ``onFinal``.
