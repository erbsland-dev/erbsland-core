.. index::
    single: TLS Client Connection
    single: TLS Configuration Labels

****************************
Using TLS Client Connections
****************************

Registering a Policy Once
=========================

TLS clients obtain their certificate policy from the application-wide cryptology configuration.
Parse trust anchors and configuration data once during startup, then register the complete TLS configuration under a
label:

.. code-block:: cpp

    auto tls = cryptology::TlsConfiguration{
        cryptology::X509ServerCertificatePolicy{trustAnchors, intermediates}};
    core::application().cryptologyConfiguration().setTlsConfiguration("tls/client"_el, std::move(tls));

The regular client default is ``tls/client``.
Resolution removes one final slash-delimited segment at a time and ends at the empty global default.
A request for ``tls/client/internal`` therefore tries ``tls/client/internal``, ``tls/client``, ``tls``, and ``""``.
The first exact entry is selected in full; missing fields are not inherited from another entry.

Each connection captures an immutable entry when ``connect()`` starts.
Replacing a registry label affects later connections without changing an in-progress connection.
Invalid labels, an unresolved chain, or a selected entry without a server-certificate policy fail synchronously and
leave the connection inactive.

Connecting and Checkpoints
==========================

Create the source on its owner event loop and install handlers before connecting:

.. code-block:: cpp

    auto connection = events->get<network::Network>().createTlsClientConnection();
    connection->events()
        .onTransportConnected([]() {
            // TCP is connected; the peer is not authenticated yet.
        })
        .onPeerHello([connection]() {
            // Cipher suite and ALPN are selected, but identity is not authenticated.
        })
        .onPeerAuthenticated([connection]() {
            // peerCertificatePath() is now authenticated.
        })
        .onHandshakeCompleted([connection]() {
            connection->send(mem::ByteBlock({'o', 'k'}));
        });
    connection->connect(network::HostEndpoint::fromStringOrThrow("example.test:443"_el));

Checkpoint callbacks continue automatically after returning.
Calling ``abort()`` inside one prevents the next protocol transition or queued transport flight.
``onPeerHello`` exposes selected parameters after EncryptedExtensions.
``onPeerAuthenticated`` follows certificate validation and CertificateVerify.
``onHandshakeCompleted`` follows verified server Finished and queued client Finished.

Back-Pressure and Receiving
===========================

Application sends are accepted only in ``Active`` and are limited to 16 KiB per atomic call.
``WouldBlock`` leaves the block with the caller; retry after ``onWritable``.
Complete encrypted records move atomically into TCP, and the facade retains at most one TCP-rejected record.

``pauseReceiving()`` stops application delivery and propagates the pause to TCP.
One aggregate TLS receive budget covers incomplete encrypted input, incomplete handshake input, and queued decrypted
application data.
Empty peer application cover records consume no queued block and produce no data event.

Deadlines and Closure
=====================

The nested TCP options keep the 30-second DNS/TCP deadline.
TLS independently defaults to a 30-second handshake deadline, five-minute authenticated idle deadline, and ten-second
graceful-close deadline.
Only accepted non-empty application sends and delivered non-empty application data reset the idle deadline; handshake,
alerts, and other TLS records do not.

``close()`` sends ``close_notify`` and waits for the peer notification.
A peer notification received first is answered before TCP closes.
Bare TCP EOF is truncation, not successful closure.
Successful TLS closure reports whether the local application or remote peer initiated it, then emits ``onFinal``.
``abort()`` emits only ``onFinal``.
