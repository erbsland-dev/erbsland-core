.. index::
    single: TLS Client Connection
    single: TLS; Server Authentication
    single: TLS; Back-Pressure

*********************************
How to Use TLS Client Connections
*********************************

A TLS client connection lets you exchange bytes with a server whose identity you have verified.
You choose the destination and the certificates you trust; the connection handles name resolution, TCP, and the TLS 1.3
handshake before delivering application data.
This page explains how to keep that connection alive, follow authentication, handle data without blocking the event
loop, and finish an exchange cleanly.

The excerpts use a command-line mail sender to illustrate an authenticated connection from startup to closure.
The TLS operations shown here apply to any application protocol that starts TLS immediately after connecting.

Connection Ownership and the Event Loop
=======================================

Create your connection from the :cpp:class:`Network <erbsland::network::Network>` API of the event loop that will own
it.
Its callbacks run on that loop, and you perform connection operations there too.
Retain the connection in an object that will live through its callbacks, and install the handlers before calling
``connect()`` so they can observe even an early failure.
In an event-driven :cpp:class:`Application <erbsland::core::Application>`, you can use the main event loop and retain
the connection as a member, as in this excerpt:

.. erbsland-demo::
    :source: network/MailSender/src/MailSenderApp.hpp
    :show: c:Retained until the event loop finishes, f:connectToServer
    :exec: network/mail_sender --verbose
    :show-cmd-line:
    :sha256: 09ee4bf6382b9f611f3f6c4757a999758557f915a3e5e0f45ee744e9fb452629

.. code-block:: cpp

    el::TlsClientConnectionPtr _connection; ///< Retained until the event loop finishes.

    void connectToServer() {
        _connection = events()->get<el::Network>().createTlsClientConnection();
        installHandlers();
        auto options = el::TlsClientConnectOptions{};
        options.setConfigurationLabel(_settings.tlsProfile);
        _mailLog->info("Connecting to "_el, _settings.endpoint.toString());
        _connection->connect(_settings.endpoint, options);
    }

.. erbsland-demo-run::

    .. code-block:: console

        $ network/mail_sender --verbose

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0m␛[97;49mConnecting to localhost:8465
        ␛[36;49mTCP connected; authenticating the server.
        Server selected cipher TLS_AES_256_GCM_SHA384
        Server certificate authenticated; path contains 2 certificates.
        TLS ready using profile tls/mail; waiting for the SMTP greeting.
        S: 220 localhost TLS mail receiver
        C: HELO localhost
        S: 250 localhost
        C: MAIL FROM:<sender@example.test>
        S: 250 Sender accepted
        C: RCPT TO:<reader@example.test>
        S: 250 Recipient accepted
        C: DATA
        S: 354 End with a line containing a single dot
        C: [message data]
        S: 250 Message received
        ␛[97;49mMessage accepted for reader@example.test
        ␛[36;49mC: QUIT
        S: 221 Bye
        TLS close_notify exchange completed.
        ␛[97;49mMail exchange completed.␛[0m

.. erbsland-demo-end::

.. dropdown:: How to run the demo

    Build the ``mail_sender`` and ``mail_receiver`` demo targets.
    The build places both executables, their default configurations, and the demonstration certificates in
    ``cmake-build-debug/demo-apps/network``.
    Start the receiver with ``--once --timeout 8`` in another terminal, then run the sender with the command shown above.
    The receiver accepts a single exchange and exits after the connection finishes.

    This is an example run: it shows transport connection, certificate authentication, application traffic, and TLS closure
    as distinct events.
    Without ``--verbose``, the trace section is disabled and only the ordinary application messages remain.
    The supplied server permits this unauthenticated SMTP transaction over authenticated TLS.
    For another SMTP service, choose an implicit-TLS endpoint that accepts your message without SMTP authentication.

If your callbacks capture ``this``, their owning object must also outlive them.
Keep that owner and its event loop available until ``onFinal`` has run.
For a single-exchange application, this callback can end the event loop with ``quit()``.
For several concurrent exchanges, it can instead schedule removal of just the completed exchange.

A connection is a one-shot source.
Create a new one for a later exchange rather than reconnecting a completed source.

Choosing Which Server to Trust
==============================

The destination host serves two purposes: it locates the server and identifies the certificate you expect.
A host name also supplies the TLS Server Name Indication (SNI).
An IP destination must match an IP identity in the certificate; it is not a way to bypass identity checking.

A TLS client profile defines which certificate authorities you trust.
For a private service, ``Trust Anchors File`` can name a PEM file containing its CA certificates.
The standard TLS configuration section expresses that choice like this:

.. code-block:: elcl

    *[TLS.Client]
    Label: "tls/mail"
    Trust Anchors File: "client-certificates/ca.pem"

Certificate paths are relative to the configuration file.
For services trusted by your operating system, you can use system trust, including the built-in default client profile.
In either case, the destination must match the authenticated certificate identity.

Load and register your profiles before starting the connection.
In the following startup callback, ``replaceTlsConfigurations()`` registers the profiles before ``events()->invoke()``
queues the connection attempt on the main loop:

.. erbsland-demo::
    :source: network/MailSender/src/MailSenderApp.hpp
    :show: f:handleCommandLineOptions
    :sha256: 5a62781b1cfe5e0ab9c2f4535968bc0536bcdb50a7704bcae1c04c00bd5f4ee2

.. code-block:: cpp

    void handleCommandLineOptions(const el::OptionValuesPtr &values) override {
        setupLogging(values);
        const auto configuration = MailConfiguration{values, "mail_sender.elcl"_el};
        _settings = SenderSettings::load(configuration);
        _messageData = _settings.mail.wireData(); // Validate all mail input before opening a socket.
        cryptologyConfiguration().replaceTlsConfigurations(configuration.tlsProfiles());
        events()->invoke([this]() -> void { connectToServer(); });
    }

.. erbsland-demo-end::

:cpp:class:`TlsConfigurationParser <erbsland::cryptology::TlsConfigurationParser>` validates the standard TLS section and loads its certificate material.
``setConfigurationLabel()`` on the connect options selects one of the registered profiles when the connection starts.
Configuration loading is synchronous, so startup is a suitable place for it.
A running service should do expensive reload work away from its responsive event loop.

Labels can fall back through their ancestors: a request for ``tls/mail/archive`` can select ``tls/mail``.
Lookup selects one complete client profile, and a server-only profile does not satisfy a client lookup.
The connection retains the selected configuration; replacing registered profiles affects later connections.
An unusable trust configuration fails before TCP starts.
See :doc:`/topics/cryptology/configuring_tls` for profile loading, system trust, and replacement rules.

Following Connection and Authentication
=======================================

A successful TCP connection tells you that you reached an endpoint, but does not authenticate its operator.
TLS adds checkpoints so you can observe the selected parameters and the verified peer identity before beginning your
application exchange.
Register these callbacks through ``events()``; logging them makes the stages visible in the following example:

.. erbsland-demo::
    :source: network/MailSender/src/MailSenderApp.hpp
    :show: f:installHandlers
    :sha256: e8c6e825ba9e8767689e74bd663b15283115f23d3e7755138603619d85ee042f

.. code-block:: cpp

    void installHandlers() {
        _connection->events()
            .onTransportConnected([this]() -> void { _mailLog->trace("TCP connected; authenticating the server."_el); })
            .onPeerHello([this]() -> void {
                _mailLog->trace("Server selected cipher "_el, _connection->cipherSuite()->toString());
            })
            .onPeerAuthenticated([this]() -> void {
                _mailLog->trace(
                    "Server certificate authenticated; path contains "_el,
                    _connection->peerCertificatePath().count(),
                    " certificates."_el);
            })
            .onHandshakeCompleted([this]() -> void {
                _mailLog->trace(
                    "TLS ready using profile "_el,
                    _connection->matchedConfigurationLabel(),
                    "; waiting for the SMTP greeting."_el);
            })
            .onData([this](el::ByteBlock data) -> void { receiveReplies(data); })
            .onWritable([this]() -> void { flushOutput(); })
            .onClosed([this](const el::ConnectionCloseContext &) -> void {
                _closedCleanly = true;
                _mailLog->trace("TLS close_notify exchange completed."_el);
            })
            .onError([this](const el::NetworkErrorContext &error) -> void {
                _failed = true;
                _mailLog->error(
                    error.title(),
                    ": "_el,
                    error.description(),
                    " (phase "_el,
                    static_cast<int>(error.phase()),
                    ", reason "_el,
                    static_cast<int>(error.reason()),
                    ")"_el);
            })
            .onFinal([this]() -> void { finish(); });
    }

.. erbsland-demo-end::

``onPeerHello`` is where negotiated parameters, such as the cipher suite and ALPN identifier, become available.
The peer identity is not authenticated at that point.
``onPeerAuthenticated`` follows certificate validation and proof that the server holds the corresponding private key;
``peerCertificatePath()`` then gives you the authenticated certificate path.
``onHandshakeCompleted`` marks the end of the handshake checkpoints.

Checkpoint callbacks continue automatically when they return.
Keep them short, and call ``abort()`` if your application decides not to continue.
When your protocol sends first, queue that initial application work with ``events()->invoke()`` so the handshake
checkpoint returns before you call ``send()``.
Keep the exchange owner alive in the queued callback, as shown for the greeting in the
:doc:`server topic <using-tls-server-connections>`.
For a protocol in which the server sends first, begin processing its greeting in ``onData`` and send your response from
there.

ALPN lets a protocol identify itself during the handshake.
Set ``alpnProtocols()`` through the connect options when your application protocol uses ALPN, and inspect
``negotiatedAlpn()`` after selection.
An empty result means no application protocol was negotiated through ALPN.

Receiving a Byte Stream
=======================

TLS protects a stream of bytes; it does not preserve your application's message boundaries.
One data callback may contain part of a reply, several replies, or a boundary in the middle of a line ending.
Retain incomplete input between callbacks and let your application protocol decide when a message is complete.

.. erbsland-demo::
    :source: network/MailSender/src/MailSenderApp.hpp
    :show: c:Partial SMTP reply line, c:Multiline reply state, f:receiveReplies
    :sha256: d6fa3d9a1f95c1edebd8216ce528d4cff72b734376eda906a4b588135b49f556

.. code-block:: cpp

    SmtpLineBuffer _lines;                  ///< Partial SMTP reply line.

    SmtpReply _reply;                       ///< Multiline reply state.

    void receiveReplies(const el::ByteBlock &data) {
        try {
            _lines.receive(data, [this](const el::String &line) -> bool {
                _mailLog->trace("S: "_el, line);
                if (const auto code = _reply.receive(line)) {
                    advanceConversation(*code);
                }
                return !_failed && _stage != Stage::Complete;
            });
        } catch (const el::ApplicationError &error) {
            fail(error.toString());
        }
    }

.. erbsland-demo-end::

Keep parsing state alongside the connection so a later callback can continue an incomplete message.
Here, ``_lines`` and ``_reply`` retain that state across calls to ``receiveReplies()``.
For another protocol, you might accumulate a length-prefixed frame or pass the bytes to a streaming parser.
``onData`` delivers only application data; TLS handles handshake traffic and alerts itself.

Sending Without Blocking
========================

``send()`` accepts a complete byte block or leaves it with you.
Each TLS application send is limited to 16 KiB, so larger messages need several calls.
A ``WouldBlock`` result means the connection currently has insufficient output capacity; retain that same block and
retry when ``onWritable`` runs.

.. erbsland-demo::
    :source: network/MailSender/src/MailSenderApp.hpp
    :show: c:Blocks waiting for TLS send capacity, f:flushOutput
    :sha256: c797c743daeca1824826c901743fdf30e5259b5f0c250fd1f8056826018f510a

.. code-block:: cpp

    PendingOutput _output;                  ///< Blocks waiting for TLS send capacity.

    void flushOutput() {
        while (!_output.isEmpty()) {
            const auto block = _output.nextBlock();
            const auto status = _connection->send(block);
            if (status.wouldBlock()) {
                return;
            }
            if (status.isClosed()) {
                fail("The connection closed with unsent SMTP data."_el);
                return;
            }
            _output.accepted(block.length());
        }
    }

.. erbsland-demo-end::

Remove a block from your queue only after ``send()`` accepts it, as ``_output.accepted()`` does here.
Accepted data belongs to the connection's output path; retrying it would duplicate bytes.
Acceptance means queued for transport, not acknowledged by the remote application.
If you need confirmation that the peer processed a request, obtain it through your application protocol.

Keep your own queue bounded as well as the connection's buffers.
For receiving, ``pauseReceiving()`` suspends application delivery and ``resumeReceiving()`` restarts it when your
consumer can make progress.
The pause propagates to the transport; data already retained still consumes buffer capacity.
The server example shows pausing input while its replies wait for output capacity.

Deadlines, Closure, and Failure
===============================

Connection establishment, the TLS handshake, application inactivity, and graceful closure have separate deadlines.
The nested TCP options default to a 30-second DNS/TCP deadline; TLS defaults to 30 seconds for the handshake, five
minutes for idle application traffic, and ten seconds for closure.
Adjust these through :cpp:class:`TlsClientConnectOptions <erbsland::network::TlsClientConnectOptions>` to fit your
service.
Accepted nonempty application sends and delivered nonempty application data reset the idle deadline.
TLS maintenance traffic does not keep an otherwise idle application exchange alive.

Certificate verification can take time, particularly with native trust services.
It runs in the background while the event loop remains available, but still counts toward the handshake deadline.
After cancellation or timeout, a late verification result cannot restart the connection.

Finish your application exchange before requesting TLS closure:

.. erbsland-demo::
    :source: network/MailSender/src/MailSenderApp.hpp
    :show: f:closeConnection, f:finish
    :sha256: f8071f32e21be31c8201c11372de0e984eef309d6cbfa0a95cfd04b46f6c4f5b

.. code-block:: cpp

    void closeConnection() {
        _stage = Stage::Complete;
        _connection->close();
    }

    void finish() {
        if (_failed || !_closedCleanly || _stage != Stage::Complete) {
            _mailLog->error("Mail exchange did not finish cleanly."_el);
            quit(el::ExitCode::failure());
            return;
        }
        _mailLog->info("Mail exchange completed."_el);
        quit();
    }

.. erbsland-demo-end::

``close()`` drains accepted output, sends ``close_notify``, and waits for the peer's notification.
If the peer starts orderly closure, TLS replies before closing TCP.
A bare TCP EOF is a truncation error, because it does not authenticate the end of the protected stream.
``onClosed`` describes successful TLS closure and its initiating side; it does not establish that your application
protocol finished successfully.
Track protocol completion separately from TLS closure; the excerpt checks both before reporting success.

Operational failures arrive through ``onError`` with a description, phase, and reason, followed by ``onFinal``.
Successful closure produces ``onClosed`` followed by ``onFinal``.
An explicit ``abort()`` skips graceful closure and still finalizes the source.
Use finalization as the common cleanup point, and report application errors separately from network failures.

Presenting a Client Certificate
===============================

Some services also authenticate clients with certificates.
Add an identity to the selected client profile; the connection presents it when the authenticated server requests a
suitable certificate.
The profile still needs trust settings for authenticating the server.
For a file-based identity, add the certificate chain and matching signing key to the client profile:

.. code-block:: elcl

    *[TLS.Client]
    Label: "tls/mail"
    Trust Anchors File: "client-certificates/ca.pem"
    Certificate Chain File: "client-certificates/client.pem"
    Signing Key File: "client-certificates/client-key.pem"

``TlsConfigurationParser`` loads the identity together with the trust settings.
The same profile-loading and connection callbacks work with or without a client identity:

.. erbsland-demo::
    :source: network/MailCommon/src/MailConfiguration.hpp
    :show: c:Load the standard TLS section(+0,+3)
    :files: ../../../demos/network/MailSender/conf/mail_sender_identity.elcl
    :exec: network/mail_sender --config mail_sender_identity.elcl --verbose
    :show-cmd-line:
    :sha256: 4f52095ab83eda38b1055b7a9b3d88e7f768a373f25cc5bcd057f9133d507c63

.. code-block:: cpp

    /// Load the standard TLS section, including its own validation rules.
    [[nodiscard]] auto tlsProfiles() const -> el::TlsConfigurationSet {
        return el::TlsConfigurationParser{}.parse(_document->value("tls"_el));
    }

.. erbsland-demo-run::

    .. code-block:: console

        $ network/mail_sender --config mail_sender_identity.elcl --verbose

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0m␛[97;49mConnecting to localhost:8465
        ␛[36;49mTCP connected; authenticating the server.
        Server selected cipher TLS_AES_256_GCM_SHA384
        Server certificate authenticated; path contains 2 certificates.
        TLS ready using profile tls/mail; waiting for the SMTP greeting.
        S: 220 localhost TLS mail receiver
        C: HELO localhost
        S: 250 localhost
        C: MAIL FROM:<sender@example.test>
        S: 250 Sender accepted
        C: RCPT TO:<reader@example.test>
        S: 250 Recipient accepted
        C: DATA
        S: 354 End with a line containing a single dot
        C: [message data]
        S: 250 Message received
        ␛[97;49mMessage accepted for reader@example.test
        ␛[36;49mC: QUIT
        S: 221 Bye
        TLS close_notify exchange completed.
        ␛[97;49mMail exchange completed.␛[0m

.. erbsland-demo-end::

.. dropdown:: How to run the demo with a client certificate

    The supplied ``mail_sender_identity.elcl`` adds a client certificate and matching private key.
    For this run, start the receiver with ``mail_receiver_sni.elcl``: its ``localhost`` mapping selects a profile requiring
    a client certificate.

Connection handling stays the same; the identity belongs to the selected TLS profile.
A server requiring client authentication rejects a client without an acceptable identity.
The :doc:`server topic <using-tls-server-connections>` explains how the server inspects the authenticated certificate
and keeps certificate authentication separate from application authorization.
