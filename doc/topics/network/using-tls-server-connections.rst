.. index::
    single: TLS Server Connection
    single: TLS; Client Authentication
    single: TLS; SNI
    single: Connection Quota

*********************************
How to Use TLS Server Connections
*********************************

A TLS server accepts TCP transports and turns them into protected application streams.
You provide its certificate and private key, choose whether clients must authenticate, and retain each connection until
its work has finished.
This page explains how those pieces fit together on an event loop, how to limit concurrent work, and how to shut down
without losing ownership of active connections.

The excerpts use a mail receiver to illustrate accepting, authenticating, and closing connections.
Its TLS session owns the connection independently of the message parser, a pattern you can use with your own application
protocol.

From Listener to TLS Session
============================

A :cpp:class:`TcpListener <erbsland::network::TcpListener>` listens on a local endpoint.
Each ``onConnection`` callback receives a pending
:cpp:class:`TcpConnectionRequest <erbsland::network::TcpConnectionRequest>` representing one accepted transport.
Create a :cpp:class:`TlsServerConnection <erbsland::network::TlsServerConnection>` for that request and pass it to
``accept()`` with the server's TLS options.
The listener remains available for later connections while each TLS source handles its own handshake and traffic.

Keep the listener and each TLS connection on their owning event loops; callbacks and connection operations run there.
A session object can retain one connection and its application state until finalization.
Store the session before starting acceptance, as the collection in this example does:

.. erbsland-demo::
    :source: network/MailReceiver/src/MailReceiverApp.hpp
    :show: c:Owns sessions through onFinal, f:acceptConnection
    :sha256: d408c1583d1bff809dd99a79509c577fae2e5152991659bc9e4abd49a33f0c69

.. code-block:: cpp

    std::map<std::size_t, std::shared_ptr<MailSession>> _sessions; ///< Owns sessions through onFinal.

    void acceptConnection(el::TcpConnectionRequestPtr request) {
        if (_stopping || (_once && !_sessions.empty())) {
            request->reject();
            return;
        }
        const auto id = ++_nextSessionId;
        auto session = std::make_shared<MailSession>(loop(), _mailLog, [this, id](const bool delivered) -> void {
            // Defer destruction until the session's final callback has returned.
            loop()->invoke([this, id, delivered]() -> void { releaseSession(id, delivered); });
        });
        _sessions.emplace(id, session);
        auto options = el::TlsServerAcceptOptions{_handshakeQuota};
        options.setConfigurationLabel(_settings.tlsProfile).setConfigurationMappings(_settings.mappings);
        session->accept(std::move(request), std::move(options));
    }

.. erbsland-demo-end::

Retaining the session keeps its connection and protocol state available throughout the asynchronous exchange.
When a session owns the source invoking its callback, defer destruction until that callback has returned.
The ``loop()->invoke()`` call above schedules removal from the collection after finalization.

Install the TLS callbacks before passing the request to ``accept()`` so they can handle both handshake progress and
early failures:

.. erbsland-demo::
    :source: network/MailReceiver/src/MailSession.hpp
    :show: c:TLS source owned until finalization, f:accept
    :sha256: d51d98912503aece65bad1f511c58607649c22dbe8584543c4c63e15c859c156

.. code-block:: cpp

    el::TlsServerConnectionPtr _connection; ///< TLS source owned until finalization.

    void accept(el::TcpConnectionRequestPtr request, el::TlsServerAcceptOptions options) {
        installHandlers();
        _connection->accept(std::move(request), std::move(options));
    }

.. erbsland-demo-end::

A request is an ownership decision, not an application stream you can read from directly.
Accept it once, or reject it when the service cannot take more work.
During shutdown, reject new requests while allowing accepted connections to finish.
For a multi-loop server, arrange for the TLS source and its operations to belong to the intended session loop.

Providing the Server Identity
=============================

Before accepting TLS connections, register a server profile containing a certificate chain and its matching private key.
The chain starts with the server certificate, followed by any intermediates needed by clients.
A file-based identity can be registered through the standard TLS configuration section:

.. code-block:: elcl

    *[TLS.Server]
    Label: "tls/mail"
    Certificate Chain File: "server-certificates/server.pem"
    Signing Key File: "server-certificates/server-key.pem"

:cpp:class:`TlsConfigurationParser <erbsland::cryptology::TlsConfigurationParser>` loads the standard TLS section, resolves certificate paths relative to the
configuration file, and checks the identity before registration.
Register the resulting profiles in your application's cryptology configuration, then select the default server profile
with ``setConfigurationLabel()`` on :cpp:class:`TlsServerAcceptOptions <erbsland::network::TlsServerAcceptOptions>`.
The :doc:`TLS profile topic </topics/cryptology/configuring_tls>` explains file identities, native identities, key
passwords, and trust settings in detail.

Unlike an ordinary client, a server cannot start from system trust alone: it needs an identity to present.
Load configuration before listening so configuration errors are visible at startup.
If you later reload profiles, an accepted connection keeps the profile snapshot selected when ``accept()`` began;
subsequent accepts use the replacement.

Completing the Handshake Before Application Work
================================================

TCP acceptance is followed by TLS negotiation.
``onClientHello`` exposes the requested server name, offered ALPN identifiers, and selected profile and parameters.
Use it to observe the client's request or reject a connection that your application policy cannot serve.
It is not evidence of an authenticated client identity.

.. erbsland-demo::
    :source: network/MailReceiver/src/MailSession.hpp
    :show: c:The receiver retains this session(+0,+1), c:Owns the queue used, f:installHandlers
    :sha256: d024d34a41543d365dad3e02c4777f321d6f63d318b8eea84810d9e70af52395

.. code-block:: cpp

    /// The receiver retains this session until a deferred notification following onFinal.
    class MailSession : public std::enable_shared_from_this<MailSession> {

    el::EventTargetPtr _events;             ///< Owns the queue used to start application work after the checkpoint.

    void installHandlers() {
        _connection->events()
            .onTransportConnected([this]() -> void { _log->trace("Accepted TCP transport; waiting for TLS."_el); })
            .onClientHello([this]() -> void { inspectClientHello(); })
            .onHandshakeCompleted([this]() -> void {
                // Let the handshake checkpoint return before sending application bytes.
                _events->invoke([self = shared_from_this()]() -> void {
                    if (self->_connection->state() == el::ConnectionState::Active) {
                        self->beginConversation();
                    }
                });
            })
            .onData([this](el::ByteBlock data) -> void { receiveCommands(data); })
            .onWritable([this]() -> void { flushOutput(); })
            .onClosed([this](const el::ConnectionCloseContext &) -> void {
                _closedCleanly = true;
                _log->trace("TLS close_notify exchange completed."_el);
            })
            .onError([this](const el::NetworkErrorContext &error) -> void {
                _log->error(
                    error.title(),
                    ": "_el,
                    error.description(),
                    " (phase "_el,
                    static_cast<int>(error.phase()),
                    ", reason "_el,
                    static_cast<int>(error.reason()),
                    ")"_el);
            })
            .onFinal([this]() -> void { _finished(_logged && _closedCleanly); });
    }

.. erbsland-demo-end::

Each handshake checkpoint continues automatically after its callback returns.
Calling ``abort()`` there prevents the next transition.
Keep callbacks short: certificate verification and private-key operations already run on background workers, and their
waiting time counts toward the handshake deadline.
Cancellation or timeout prevents a late background result from restarting the exchange.

When your protocol sends first, schedule that work from ``onHandshakeCompleted`` with ``loop()->invoke()``.
This lets the handshake checkpoint return before the first application send.
Retain the session in the queued callback and check that the connection is still active, because shutdown may occur
before the queued work runs.
The excerpt above uses this pattern to call ``beginConversation()``, which checks peer authentication and sends the
first application bytes:

.. erbsland-demo::
    :source: network/MailReceiver/src/MailSession.hpp
    :show: f:beginConversation
    :exec: network/mail_receiver --once --timeout 8 --verbose
    :show-cmd-line:
    :sha256: 2c1bc85f191fbb8128d9efefff0031b670c0a1524a04c937e2a55cb3c4fdbef2

.. code-block:: cpp

    void beginConversation() {
        if (_connection->isPeerAuthenticated()) {
            _log->info(
                "Client certificate authenticated; path contains "_el,
                _connection->peerCertificatePath().count(),
                " certificates."_el);
        } else {
            _log->trace("TLS ready; client did not present an authenticated certificate."_el);
        }
        sendReply("220 localhost TLS mail receiver\r\n"_el);
    }

.. erbsland-demo-run::

    .. code-block:: console

        $ network/mail_receiver --once --timeout 8 --verbose

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0m␛[97;49mListening on 127.0.0.1:8465
        ␛[36;49mAccepted TCP transport; waiting for TLS.
        Client requested server name localhost
        Selected server profile tls/mail with cipher TLS_AES_256_GCM_SHA384
        TLS ready; client did not present an authenticated certificate.
        S: 220 localhost TLS mail receiver
        S: 250 localhost
        S: 250 Sender accepted
        S: 250 Recipient accepted
        S: 354 End with a line containing a single dot
        ␛[97;49mReceived mail from sender@example.test for reader@example.test
        Date: 11 Nov 2026 16:27:47 +0000
        From: "Alex" <sender@example.test>
        To: "Sam" <reader@example.test>

        The new observation notes are ready.
        See you at the telescope tonight!
        ␛[36;49mS: 250 Message received
        S: 221 Bye
        TLS close_notify exchange completed.
        ␛[97;49mStopping the mail receiver.␛[0m

.. erbsland-demo-end::

.. dropdown:: How to run the demo

    Build the ``mail_receiver`` and ``mail_sender`` targets.
    Their executables and configuration assets are placed in ``cmake-build-debug/demo-apps/network``.
    Start the receiver with the command shown above, then run ``mail_sender`` in another terminal.
    The ``--once`` option makes this example finish after one successful exchange; ``--timeout`` bounds the wait if no
    sender arrives.

    The trace output shows when the transport is accepted, the TLS profile is selected, and application traffic begins.
    The received message is ordinary application output.
    Without ``--verbose``, the trace section is disabled.
    An optional ``Main.Log File`` appends the log to a file as well as writing to the console.
    Relative log paths start beside the selected configuration file.

    Without ``--once``, the receiver keeps serving and accepts one message on each connection.
    The supplied configuration listens on loopback.

Once the handshake completes, ``onData`` delivers application bytes and ``send()`` queues protected output.
TLS record boundaries are not message boundaries: a callback can deliver part of a message or several messages at once.
Retain incomplete input between deliveries and let your application protocol determine when a message is complete.

Bounding Connections and Handshakes
===================================

An established connection and an incomplete handshake consume different resources.
Use a TCP connection quota to bound accepted transports, and a separate TLS handshake quota to bound the work needed to
establish new protected streams.

.. erbsland-demo::
    :source: network/MailReceiver/src/MailReceiverApp.hpp
    :show: f:startListener
    :sha256: f50a1dea4d5d78bbfd5cb5db6986397fe067f4669a5cce279779f2b609318b59

.. code-block:: cpp

    void startListener() {
        _connectionQuota = el::ConnectionQuota::create(el::ItemCount{16});
        _handshakeQuota = el::ConnectionQuota::create(el::ItemCount{4});
        _listener = loop()->get<el::Network>().createTcpListener();
        _listener->events()
            .onListening(
                [this]() -> void { _mailLog->info("Listening on "_el, _listener->localEndpoint()->toString()); })
            .onConnection([this](el::TcpConnectionRequestPtr request) -> void { acceptConnection(std::move(request)); })
            .onError([this](const el::NetworkErrorContext &error) -> void {
                _mailLog->error(
                    error.title(),
                    ": "_el,
                    error.description(),
                    " (phase "_el,
                    static_cast<int>(error.phase()),
                    ", reason "_el,
                    static_cast<int>(error.reason()),
                    ")"_el);
                _exitCode = el::ExitCode::failure();
                stopServer();
            })
            .onFinal([this]() -> void {
                _listenerFinished = true;
                finishIfStopped();
            });
        _listener->start(_settings.endpoint, el::TcpListenerOptions{}.setConnectionQuota(_connectionQuota));
        if (_timeoutSeconds > 0) {
            _timeout = loop()->createTimer([this]() -> void { stopServer(); });
            _timeout->startOnce(el::TimeDelta::seconds(_timeoutSeconds));
        }
    }

.. erbsland-demo-end::

Pass the TCP quota to ``TcpListenerOptions`` and the handshake quota to each connection's ``TlsServerAcceptOptions``.
Retain the quota objects so subsequent accepts use the same capacity limits.
A handshake lease is released when the handshake finishes, while the transport continues to count toward the TCP limit
until finalization.
Sharing either quota object with another listener combines that limit across the listeners.
Separate objects give independent limits.

When the TCP quota is full, the listener does not deliver another request and suspends acceptance until capacity becomes
available.
When the handshake quota is full, TLS acceptance reports ``ResourceLimitExceeded`` in the ``Accepting`` phase.
Handle such operational failures through the session's error callback and release its owner at finalization.
Do not make one rejected connection stop an otherwise healthy service.

Flow Control and Bounded Input
==============================

A client may send faster than your application can process, or read responses slower than you can produce them.
Bound your protocol's retained input and output as well as the TLS and TCP buffers.
TLS buffer limits bound what the connection retains, but they do not bound a message assembled across callbacks.
Enforce your protocol's size limits as you accumulate input, and abort only the affected connection when a message is
malformed or too large.

For output, each call to ``send()`` accepts at most 16 KiB atomically.
A ``WouldBlock`` result leaves the block with the caller; ``onWritable`` tells you when to retry.
For a request-response protocol, you can also call ``pauseReceiving()`` while responses wait for output capacity.
This prevents further requests from adding work to a blocked output queue.
The following excerpt combines that pause with retaining and retrying the unaccepted block:

.. erbsland-demo::
    :source: network/MailReceiver/src/MailSession.hpp
    :show: c:Replies waiting for send capacity, f:flushOutput
    :sha256: 9adbc2a06f768e929ca63a5922476c8f26d3b9efdd86cb714dacd6e79e254759

.. code-block:: cpp

    PendingOutput _output;                  ///< Replies waiting for send capacity.

    void flushOutput() {
        while (!_output.isEmpty()) {
            const auto block = _output.nextBlock();
            const auto status = _connection->send(block);
            if (status.wouldBlock()) {
                _connection->pauseReceiving();
                return;
            }
            if (status.isClosed()) {
                _connection->abort();
                return;
            }
            _output.accepted(block.length());
        }
        if (_closing) {
            _connection->close();
        } else {
            _connection->resumeReceiving();
        }
    }

.. erbsland-demo-end::

After ``send()`` accepts a block, remove it from your pending output.
Once the queue drains, call ``resumeReceiving()`` to continue processing requests, or ``close()`` if the exchange is
complete.
Pausing delivery also pauses the transport, but does not remove bytes already retained by TLS or your parser.
Continue enforcing application limits for the input currently being processed.

Selecting Profiles with SNI
===========================

When several services share an endpoint, SNI lets a client request a host name during the handshake.
Map that name to a TLS configuration label in ``TlsServerAcceptOptions``.
Keep a default profile for clients that send no recognized name.
The mapping is exact after host-name canonicalization; it is not a wildcard routing rule.

Each :cpp:class:`TlsConfigurationMapping <erbsland::network::TlsConfigurationMapping>` associates a host name with a
registered profile label.
Pass those mappings to the accept options before accepting the request:

.. code-block:: cpp

    auto mapping = el::TlsConfigurationMapping{
        el::HostName::fromStringOrThrow("localhost"_el), "tls/mail/identified"_el};
    options.setConfigurationMappings({mapping});

All mapped profiles and the default are resolved together when ``accept()`` starts.
Selection chooses a complete server profile, including its identity and client-authentication policy.
At ``onClientHello``, ``serverName()`` exposes the requested name and ``matchedConfigurationLabel()`` identifies the
selected profile, as the following excerpt demonstrates:

.. erbsland-demo::
    :source: network/MailReceiver/src/MailSession.hpp
    :show: f:inspectClientHello
    :files: ../../../demos/network/MailReceiver/conf/mail_receiver_sni.elcl
    :exec: network/mail_receiver --config mail_receiver_sni.elcl --once --timeout 8 --verbose
    :show-cmd-line:
    :sha256: 20dec53166cf4605ee0d179bf7572f3afad4d1c9ec67db354eb3c12f78df262a

.. code-block:: cpp

    void inspectClientHello() {
        if (const auto name = _connection->serverName()) {
            _log->trace("Client requested server name "_el, name->toString());
        }
        _log->trace(
            "Selected server profile "_el,
            *_connection->matchedConfigurationLabel(),
            " with cipher "_el,
            _connection->cipherSuite()->toString());
    }

.. erbsland-demo-run::

    .. code-block:: console

        $ network/mail_receiver --config mail_receiver_sni.elcl --once --timeout 8 --verbose

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0m␛[97;49mListening on 127.0.0.1:8465
        ␛[36;49mAccepted TCP transport; waiting for TLS.
        Client requested server name localhost
        Selected server profile tls/mail/identified with cipher TLS_AES_256_GCM_SHA384
        ␛[97;49mClient certificate authenticated; path contains 2 certificates.
        ␛[36;49mS: 220 localhost TLS mail receiver
        S: 250 localhost
        S: 250 Sender accepted
        S: 250 Recipient accepted
        S: 354 End with a line containing a single dot
        ␛[97;49mReceived mail from sender@example.test for reader@example.test
        Date: 11 Nov 2026 16:27:48 +0000
        From: "Alex" <sender@example.test>
        To: "Sam" <reader@example.test>

        The new observation notes are ready.
        See you at the telescope tonight!
        ␛[36;49mS: 250 Message received
        S: 221 Bye
        TLS close_notify exchange completed.
        ␛[97;49mStopping the mail receiver.␛[0m

.. erbsland-demo-end::

.. dropdown:: How to run the SNI demo

    The ``mail_receiver_sni.elcl`` configuration maps ``localhost`` to ``tls/mail/identified``, which requires a client
    certificate; the default ``tls/mail`` profile does not.
    Start the receiver with the command shown above, then run ``mail_sender --config mail_sender_identity.elcl`` from
    the directory containing the executables and configurations.

    The selected label in the trace makes the routing decision visible.
    To exercise default-profile selection, connect to ``127.0.0.1`` with the sender's ``--smtp-host`` override instead of
    using ``localhost``.
    The supplied server certificate covers both identities, so this changes SNI selection without weakening certificate
    verification.

For protocols using ALPN, configure the server's supported identifiers through ``setAlpnProtocols()`` and inspect
``negotiatedAlpn()`` after selection.
ALPN chooses the application protocol; SNI chooses the server profile.

Authenticating Clients
======================

Server authentication protects the client from connecting to an impostor.
Client-certificate authentication gives the server a verified identity for the connecting peer as well.
Configure the selected server profile with trusted client authorities and a required or optional authentication policy.
For example, these fields in a ``TLS.Server`` profile require a client certificate trusted by the specified CA:

.. code-block:: elcl

    Client Authentication: "required"
    Trust Anchors File: "server-certificates/ca.pem"

With ``required`` authentication, a missing or unacceptable client identity fails the handshake.
With ``optional`` authentication, a client may omit its certificate, but a presented certificate must still validate.

After ``onHandshakeCompleted``, ``isPeerAuthenticated()`` tells you whether the client authenticated with a certificate.
``peerCertificatePath()`` contains its validated path, starting with the client certificate.
Check that result before starting work that requires an authenticated client.
You can associate the certificate with an application account and check its permissions before allowing privileged
operations.
TLS proves identity and possession of a private key; your application determines what that identity may do.

.. dropdown:: How to run the client-authentication demos

    The supplied ``mail_receiver_required.elcl`` requires client authentication for every connection.
    Use ``mail_sender_identity.elcl`` with it; the sender's ordinary configuration is rejected because it has no identity.
    The ``mail_receiver_optional.elcl`` variant admits clients without certificates but still rejects invalid certificates
    when presented.

For an anonymous client, ``isPeerAuthenticated()`` is false and the certificate path is empty.
A completed handshake alone is therefore not a client identity.

Finishing Sessions and Stopping the Service
===========================================

Finish application-level replies before requesting TLS closure.
``close()`` drains output already accepted by TLS, exchanges ``close_notify``, and closes the transport.
An orderly peer close is answered automatically.
A peer that closes TCP without its TLS notification produces a truncation error instead of successful closure.

``onClosed`` reports orderly closure and its origin, followed by ``onFinal``.
Failures produce ``onError`` followed by ``onFinal``; an explicit abort also finalizes the source.
Use ``onFinal`` to schedule release of the session owner on every termination path, as in the acceptance example.

Stopping the service has an additional ownership requirement: its event loop must continue running while the listener
and sessions finish.
Stop accepting first, ask sessions to end, and quit only after their final callbacks have been processed.

.. erbsland-demo::
    :source: network/MailReceiver/src/MailReceiverApp.hpp
    :show: f:stopServer, f:finishIfStopped
    :sha256: 36db15e4fadf4f67e151cdcf42033c1f6bbb0ce83ee708edf23f89e63e713ffc

.. code-block:: cpp

    void stopServer() {
        if (_stopping) {
            return;
        }
        _stopping = true;
        _mailLog->info("Stopping the mail receiver."_el);
        _listener->close();
        for (const auto &[id, session] : _sessions) {
            session->stop();
        }
        _shutdownDeadline = loop()->createTimer([this]() -> void {
            for (const auto &[id, session] : _sessions) {
                session->abort();
            }
        });
        _shutdownDeadline->startOnce(el::TimeDelta::seconds(2));
        finishIfStopped();
    }

    void finishIfStopped() {
        if (_stopping && _listenerFinished && _sessions.empty()) {
            quit(_exitCode);
        }
    }

.. erbsland-demo-end::

Choose which application work to finish during shutdown, then bound how long you wait for it.
In this excerpt, the timer gives sessions two seconds to finalize before remaining connections are aborted.
Your shutdown budget is separate from the TLS handshake, idle, and close deadlines; choose it to suit the work you allow
to finish.
For normal sessions, :cpp:class:`TlsServerAcceptOptions <erbsland::network::TlsServerAcceptOptions>` defaults to a
30-second handshake deadline, a five-minute idle deadline, and a ten-second graceful-close deadline.
Only accepted nonempty application sends and delivered nonempty application data reset the idle deadline.
Choose these limits for the work your service performs, while keeping finalization on the live owner loop.
