..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: TCP; Connections
    single: Network; TCP

*********************
Using TCP Connections
*********************

TCP gives your application a reliable, ordered stream of bytes.
It does not preserve the boundaries between your ``send()`` calls, though.
A small request can arrive in several ``onData()`` callbacks, or several requests can arrive together.
This page walks through a small map service that handles that detail correctly while remaining useful to run from a
terminal.

The :file:`TcpMapServer` reads text keys and values from an ELCL document and stays running for as many client requests
as you want to make.
:file:`TcpMapClient` looks up one key at a time.
Together, the demos show how to load application data, accept connections, frame a TCP message, and finish the
connection cleanly.

TCP Carries a Stream, Not Requests
==================================

Before writing a TCP protocol, decide how the receiver can find one complete message in the stream.
The map demos use ``ByteTextOptions`` to put an unsigned four-byte, big-endian UTF-8 length in front of every key and
response, with a 4096-byte payload limit.
The receiver asks ``ByteReader`` for the complete optional text frame and waits when the TCP buffer still contains only
a prefix.
This is deliberately a small protocol, but the pattern works equally well for larger application messages.

The connection created by :cpp:class:`Network <erbsland::network::Network>` is initially inactive.
An outgoing client uses :cpp:func:`TcpConnection::connect <erbsland::network::TcpConnection::connect>`; a server accepts
an incoming
:cpp:class:`TcpConnectionRequest <erbsland::network::TcpConnectionRequest>` into the same connection type. In both
cases, configure all callbacks before starting the operation.
The event loop then delivers every later state change and data chunk on the connection's owner loop.

Configuring the Example TCP Lookup Service
==========================================

The server takes the path to an ELCL document as its required positional argument.
Its ``map`` section uses text names, which makes keys exact text instead of normalized ELCL identifiers.
The values must also be text.

.. code-block:: elcl

    [map]
    "Zurich": "Switzerland"
    "Bern": "Switzerland"
    "Geneva": "Switzerland"

You can use the bundled :file:`map.elcl` as a starting point and add your own entries.
The server parses the file before it starts listening, then copies every text-name/text-value pair into a read-only
string map shared by its sessions.
A bad map file therefore fails early, before a client can receive a partial or surprising answer.

.. erbsland-demo::
    :source: network/TcpMapServer/TcpMapServerApp.hpp
    :show: f:registerCommandLineOptions, f:handleCommandLineOptions, f:loadMap
    :sha256: ceaf825d09da941e01e8ea8e770709855a106457df6810706df8f2a79ca478d0

.. code-block:: cpp

    void registerCommandLineOptions(const el::OptionsPtr &options) override {
        options->setHelpDescription("Serves text values from an ELCL map over TCP."_el);
        options->addOption("map-file"_el).setRequired().setHelpTitle("ELCL document containing the map section."_el);
        options->addOption({"-a"_el, "--address"_el, "address"_el})
            .setType(el::OptionType::Text)
            .setDefaultValue("127.0.0.1"_el)
            .setHelpTitle("Local IP address to bind."_el)
            .setValidateTextValue<el::IpAddress>("The address is not a valid IP address"_el);
        options->addOption({"-p"_el, "--port"_el, "port"_el})
            .setType(el::OptionType::Text)
            .setDefaultValue("45870"_el)
            .setHelpTitle("Local TCP port to bind."_el)
            .setValidateTextValue<el::Port>("The port is not valid"_el);
    }

    void handleCommandLineOptions(const el::OptionValuesPtr &values) override {
        _address = el::IpAddress::fromStringOrThrow(values->getText("address"_el));
        _port = el::Port::fromStringOrThrow(values->getText("port"_el));
        _map = loadMap(el::Path::fromNativeOrThrow(values->getText("map-file"_el)));
        events()->invoke([this]() -> void { startServer(); });
    }

    auto loadMap(const el::Path &path) -> std::shared_ptr<const Map> {
        const auto document = el::conf::Parser{}.parseFileOrThrow(path);
        const auto mapSection = document->getSectionWithTextsOrThrow("map"_el);
        auto map = Map{};
        for (const auto &entry : *mapSection) {
            map.set(entry->name().asText(), entry->asTextOrThrow());
        }
        return std::make_shared<Map>(std::move(map));
    }

.. erbsland-demo-end::

Start the service with the map file and, if necessary, choose another local address or port:

.. code-block:: shell

    tcp_map_server map.elcl
    tcp_map_server --address 127.0.0.1 --port 45871 map.elcl

Listen and Accept Connections
=============================

A :cpp:class:`TcpListener <erbsland::network::TcpListener>` binds an
:cpp:class:`IpEndpoint <erbsland::network::IpEndpoint>`. Once ``onListening()`` runs,
:cpp:func:`TcpListener::localEndpoint <erbsland::network::TcpListener::localEndpoint>` is available, which is useful
when you let the operating system choose a port.

The demo creates one :cpp:class:`TcpConnection <erbsland::network::TcpConnection>` per accepted request.
The listener filter only performs a quick loopback check; it does not read data or do other work that could delay
accepting another connection.
Each session reports the remote endpoint and requested key in colour, so you can see which client made each lookup while
you experiment with the client.

.. erbsland-demo::
    :source: network/TcpMapServer/TcpMapServerApp.hpp
    :show: f:startServer, f:startSession, f:removeSession
    :sha256: 31478d2096bdb5095c1699cb75a9d04909078e1c7d102e4730100e4ff39c4b91

.. code-block:: cpp

    void startServer() {
        terminal()->printLine(fg::BrightWhite, BlockAttributes::Bold, "TCP Map Server"_el);
        terminal()->printLine(fg::Cyan, el::StringFormat{"Loaded {} map entries"_el}.build(_map->count().toSizeT()));
        _listener = events()->get<el::Network>().createTcpListener();
        _listener->events()
            .onListening([this]() -> void {
                terminal()->printLine(
                    fg::BrightGreen, "Listening on "_el, _listener->localEndpoint().value().toString());
            })
            .onConnection([this](el::TcpConnectionRequestPtr request) -> void { startSession(std::move(request)); })
            .onError([](const el::NetworkErrorContext &error) -> void { throw el::NetworkError{error}; });
        auto options = el::TcpListenerOptions{};
        options.setConnectionFilter([this](const el::IpEndpoint &remote) {
            return !_address.isLoopback() || remote.address().isLoopback() ? el::TcpConnectionFilterResult::Accept
                                                                           : el::TcpConnectionFilterResult::Reject;
        });
        _listener->start(el::IpEndpoint{_address, _port}, options);
    }

    void startSession(el::TcpConnectionRequestPtr request) {
        auto session = std::make_shared<MapSession>(
            events(),
            _map,
            std::move(request),
            [this](const el::IpEndpoint &remote, const el::String &key) -> void {
                terminal()->printLine(fg::BrightGreen, "Request from "_el, remote.toString(), ": "_el, key);
            },
            [this](MapSession *finished) -> void { removeSession(finished); });
        _sessions.emplace_back(session);
        session->start();
    }

    void removeSession(MapSession *finished) {
        events()->invoke([this, finished]() -> void {
            std::erase_if(_sessions, [finished](const auto &session) -> bool { return session.get() == finished; });
        });
    }

.. erbsland-demo-end::

The request object owns the accepted socket until
:cpp:func:`TcpConnection::accept <erbsland::network::TcpConnection::accept>` claims it.
Keeping a session object in ``_sessions`` gives that connection an explicit owner until its ``onFinal()`` callback
removes the session.
This lets the listener continue accepting new clients while earlier clients are still being handled.

Server-Side TCP Message Framing and Responses
=============================================

``onData()`` receives arbitrary byte chunks.
The session appends each chunk to its input buffer and returns until the length prefix and all payload bytes are
present.
It then decodes the key as UTF-8, logs the remote endpoint and key, and looks up the value.
If the key is absent, the protocol still returns a normal text response: ``Key not found``.

After queuing the framed reply, the server calls
:cpp:func:`TcpConnection::close <erbsland::network::TcpConnection::close>`.
``close()`` stops new sends but drains the accepted response before closing the TCP connection.
If the finite output queue cannot accept the reply, this small service aborts instead; a production protocol could
retain the block and retry it from ``onWritable()``.

.. erbsland-demo::
    :source: network/TcpMapServer/TcpMapServerApp.hpp
    :show: f:start, f:onData, f:sendAnswer
    :sha256: 8f630d216409afd3e74a6413dc13e8286955f2c7ecfdb998be75891e4607e0b6

.. code-block:: cpp

    void start() {
        _connection = _events->get<el::Network>().createTcpConnection();
        _connection->events()
            .onData([this](el::ByteBlock data) -> void { onData(std::move(data)); })
            .onError([](const el::NetworkErrorContext &error) -> void { throw el::NetworkError{error}; })
            .onFinal([this]() -> void { _done(this); });
        auto options = el::TcpAcceptOptions{};
        options.setBufferLimits(el::SocketBufferLimits{el::ByteCount{8192U}, el::ByteCount{8192U}});
        _connection->accept(std::move(_request), options);
    }

    void onData(el::ByteBlock data) {
        _input.append(data);
        auto reader = el::ByteReader{_input};
        reader.setEndianness(el::Endianness::Big);
        const auto key = reader.readText(textOptions());
        if (!key.has_value()) {
            return;
        }
        _requestFn(_connection->remoteEndpoint().value(), *key);
        sendAnswer(_map->get(*key, "Key not found"_el));
    }

    void sendAnswer(const el::String &answer) {
        auto writer = el::ByteWriter{};
        writer.setEndianness(el::Endianness::Big);
        writer.writeTextOrThrow(answer, textOptions());
        if (!_connection->send(writer.toByteBlock()).isAccepted()) {
            _connection->abort();
            return;
        }
        _connection->close();
    }

.. erbsland-demo-end::

Connect from the Client
=======================

The client accepts exactly one positional argument: the key to look up.
It connects to ``127.0.0.1:45870`` by default, so a local request is as short as this:

.. code-block:: shell

    tcp_map_client Zurich
    tcp_map_client Atlantis

Use ``--host`` and ``--port`` when the server is elsewhere.
The client creates an inactive connection from the event loop's :cpp:class:`Network <erbsland::network::Network>`
facade, registers its handlers, and then calls ``connect()``.
For a host name, TCP resolves all candidate endpoints first.
The resolution callback is the right place to apply an address policy before a native connection socket exists; this
demo rejects unspecified addresses.

.. erbsland-demo::
    :source: network/TcpMapClient/TcpMapClientApp.hpp
    :show: f:registerCommandLineOptions, f:handleCommandLineOptions, f:startClient, f:validateResolvedEndpoints
    :sha256: b1a23f6c453c98e27ee3c5f6c231335610fcdcf3d8f73be0ffbf1e9aeca16ef7

.. code-block:: cpp

    void registerCommandLineOptions(const el::OptionsPtr &options) override {
        options->setHelpDescription("Looks up one text key on a TCP map server."_el);
        options->addOption("key"_el).setRequired().setHelpTitle("Text key to look up."_el);
        options->addOption({"--host"_el, "host"_el})
            .setType(el::OptionType::Text)
            .setDefaultValue("127.0.0.1"_el)
            .setHelpTitle("Map server host name or IP address."_el)
            .setValidateTextValue<el::Host>("The host is not a valid host name or IP address"_el);
        options->addOption({"-p"_el, "--port"_el, "port"_el})
            .setType(el::OptionType::Text)
            .setDefaultValue("45870"_el)
            .setHelpTitle("Map server TCP port."_el)
            .setValidateTextValue<el::Port>("The port is not valid"_el);
    }

    void handleCommandLineOptions(const el::OptionValuesPtr &values) override {
        _key = values->getText("key"_el);
        _host = el::Host::fromStringOrThrow(values->getText("host"_el));
        _port = el::Port::fromStringOrThrow(values->getText("port"_el));
        events()->invoke([this]() -> void { startClient(); });
    }

    void startClient() {
        terminal()->printLine(fg::BrightWhite, BlockAttributes::Bold, "TCP Map Client"_el);
        terminal()->printLine(fg::Cyan, "Looking up "_el, _key, " on "_el, _host.toString());
        _connection = events()->get<el::Network>().createTcpConnection();
        _connection->events()
            .onHostResolved(
                [this](const el::List<el::IpEndpoint> &endpoints) -> void { validateResolvedEndpoints(endpoints); })
            .onConnected([this]() -> void { sendQuery(); })
            .onData([this](el::ByteBlock data) -> void { onData(std::move(data)); })
            .onClosed([this](const el::ConnectionCloseContext &) -> void {
                if (!_receivedAnswer) {
                    throw el::ApplicationError{"The map server closed without a complete response."_el};
                }
            })
            .onError([](const el::NetworkErrorContext &error) -> void { throw el::NetworkError{error}; })
            .onFinal([this]() -> void { quit(); });
        _connection->connect(el::HostEndpoint{_host, _port});
    }

    void validateResolvedEndpoints(const el::List<el::IpEndpoint> &endpoints) {
        for (const auto &endpoint : endpoints) {
            if (endpoint.address().isAny()) {
                _connection->abort();
                throw el::ApplicationError{"The map server resolved to an unspecified IP address."_el};
            }
        }
    }

.. erbsland-demo-end::

Send and Receive One Framed Message
===================================

``onConnected()`` is the first point at which the client sends its framed key.
Just as on the server, the response is accumulated in an input buffer because TCP may split it across callbacks.
Once all bytes are available, the client decodes and prints the value in colour.
The server then closes the connection, and ``onFinal()`` ends this one-shot client process.

.. erbsland-demo::
    :source: network/TcpMapClient/TcpMapClientApp.hpp
    :show: f:sendQuery, f:onData
    :sha256: aa0864e2e6f62357a4140b57a7bd43ad90c5ee195bdd1221ed6f3dde15e11a7e

.. code-block:: cpp

    void sendQuery() {
        auto writer = el::ByteWriter{};
        writer.setEndianness(el::Endianness::Big);
        writer.writeTextOrThrow(_key, textOptions());
        if (!_connection->send(writer.toByteBlock()).isAccepted()) {
            throw el::ApplicationError{"The map query did not fit in the connection output queue."_el};
        }
        terminal()->printLine(fg::BrightBlack, "Waiting for the map server"_el);
    }

    void onData(el::ByteBlock data) {
        _input.append(data);
        auto reader = el::ByteReader{_input};
        reader.setEndianness(el::Endianness::Big);
        const auto answer = reader.readText(textOptions());
        if (!answer.has_value()) {
            return;
        }
        terminal()->printLine(fg::BrightGreen, "Map value: "_el, *answer);
        _receivedAnswer = true;
    }

.. erbsland-demo-end::

The ``onClosed()`` handler also checks that a complete response was received.
This makes an early peer close visible to the person running the utility instead of presenting an incomplete lookup as a
successful one.
Network failures follow the same path through ``onError()`` and then ``onFinal()``, so the connection has one clear
terminal lifecycle.
