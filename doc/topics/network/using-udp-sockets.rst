..
    Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: UDP; Packets
    single: Network; UDP
    single: Datagram Back-Pressure
    single: Datagram; Dropped

*********************************
Sending and Receiving UDP Packets
*********************************

UDP is a datagram transport: each send creates an independent packet, and the receiver gets that packet together with
the sender's endpoint.
A :cpp:class:`UdpSocket <erbsland::network::UdpSocket>` represents one local endpoint, but it is not tied to one remote
peer.
That simplicity makes UDP useful for discovery, telemetry, and small request-and-response protocols whose design can
tolerate lost, duplicated, or reordered packets.

This page uses the RFC 868 :file:`TimeServer` and :file:`TimeClient` demos to show you how to create a socket, send only
after binding, validate replies, and close a completed exchange.
You will also see how to respond when sending is temporarily blocked and how to pause packet delivery safely.

One Socket, Many Packets
========================

Create the socket through the :cpp:class:`Network <erbsland::network::Network>` facade of the event loop that owns it,
then configure its handlers before calling ``start()``.
Binding and all later callbacks are asynchronous and run on that event loop.
Think of the socket as a reusable mailbox rather than a connection: receiving or sending one packet does not finish it,
and the same socket can communicate with many remote endpoints during its lifetime.

.. mermaid::

    sequenceDiagram
        participant App as Application
        participant Socket as UdpSocket
        participant Network as Network

        App->>Socket: start(...)
        Socket-->>App: onBound()
        Network->>Socket: UDP packet
        Socket-->>App: onDatagram(datagram)
        App->>Socket: send(remoteEndpoint, data)
        Socket->>Network: UDP packet

The :cpp:func:`UdpSocket::start <erbsland::network::UdpSocket::start>` overloads let you choose the local endpoint.
The no-argument and port-only overloads bind IPv4-any; an omitted or automatic port lets the operating system choose a
free port.
Pass :cpp:func:`IpAddress::anyV6 <erbsland::network::IpAddress::anyV6>` for IPv6-any, or a concrete address such as
:cpp:func:`IpAddress::loopbackV4 <erbsland::network::IpAddress::loopbackV4>` to restrict the socket to one interface.
An IPv6 socket is IPv6-only, so its destinations must use the same address family.

Receive and Reply at a Service
==============================

The time server creates its socket, registers its lifecycle and packet handlers, and then binds the address and port
from the command line.
Once binding completes, ``onBound()`` is the first callback in which
:cpp:func:`UdpSocket::localEndpoint <erbsland::network::UdpSocket::localEndpoint>` is available.
That makes ``onBound()`` the natural place to publish an automatically selected port or start the next step of a
request-and-response exchange.

.. erbsland-demo::
    :source: network/TimeServer/TimeServerApp.hpp
    :function-blocks: startServer onBound onDatagram
    :function-blocks-sha256: b7bd63d33186829f1d62ca4384186f4da758b0fd08c580b58664f407a0353ef2
    :source-sha256: 0f05d7fc8731d9aabace0d112bc10bddc6884df6271f826b0bf7de1ffb286144

.. code-block:: cpp

    void startServer() {
        terminal()->printLine(fg::BrightWhite, BlockAttributes::Bold, "Time Server"_el);
        terminal()->printLine(fg::Cyan, "Starting "_el, el::IpEndpoint{_address, _port}.toString());
        _socket = events()->get<el::Network>().createUdpSocket();
        _socket->events()
            .onBound([this]() -> void { onBound(); })
            .onDatagram([this](const el::UdpDatagram &datagram) -> void { onDatagram(datagram); })
            .onError([this](const el::NetworkErrorContext &error) -> void { onError(error); });
        _socket->start(_address, _port);
    }

    void onBound() {
        terminal()->printLine(fg::BrightGreen, "Listening on "_el, _socket->localEndpoint().value().toString());
    }

    void onDatagram(const el::UdpDatagram &datagram) {
        uint8_t version = 1;
        if (datagram.data().length() > el::ByteLength{1}) {
            return;
        }
        if (!datagram.data().isEmpty()) {
            version = datagram.data().getIntegerOrThrow<uint8_t>(el::ByteIndex::zero());
            if (version < 1U || version > 2U) {
                return;
            }
        }
        const auto time = el::DateTime::now();
        const auto ticks = time.toTicks<el::Seconds>(el::TimeEpoch::Rfc868);
        if (!ticks.has_value()) {
            // Stop the server for this unexpected error.
            throw el::ApplicationError{"The current date/time cannot be represented as an RFC 868 timestamp."_el};
        }
        auto response = el::ByteBlockEditor{};
        if (version == 1) {
            try {
                const auto timestamp = ticks.value().toValue().castOrThrow<std::uint32_t>();
                response.appendInteger(timestamp.toRawValue(), el::Endianness::Big);
            } catch (const el::Exception &) {
                return; // ignore the request if we cannot represent the timestamp
            }
        } else {
            const auto timestamp = ticks.value().toValue().castOrThrow<std::uint64_t>();
            response.appendInteger(timestamp.toRawValue(), el::Endianness::Big);
        }
        if (!_socket->send(datagram.remoteEndpoint(), response).isAccepted()) {
            return; // Ignore a full queue and skip this request.
        }
        terminal()->printLine(
            el::cterm::fg::BrightGreen,
            "Received request from "_el,
            datagram.remoteEndpoint().toString(),
            ", replied with "_el,
            time.toIsoString());
    }

.. erbsland-demo-end::

Inside ``onDatagram()``, the server reads the request from
:cpp:func:`UdpDatagram::data <erbsland::network::UdpDatagram::data>` and uses
:cpp:func:`UdpDatagram::remoteEndpoint <erbsland::network::UdpDatagram::remoteEndpoint>` as the reply address.
Because UDP has no connected peer, the endpoint carried by each packet is the information a server needs to send its
reply.

The :cpp:class:`UdpSocketOptions <erbsland::network::UdpSocketOptions>` passed to ``start()`` apply for the complete
socket lifetime.
Use ``maximumDatagramSize`` to set the largest payload your protocol accepts.
It limits both incoming and outgoing payloads and defaults to the portable UDP payload maximum of 65,507 bytes.
Choose a limit that fits the networks your application must use instead of relying on that theoretical maximum.

Send Only After Binding
=======================

Before it can send, the client resolves the host name because ``send()`` takes an
:cpp:class:`IpEndpoint <erbsland::network::IpEndpoint>`, not a host name.
See :doc:`resolving-hosts-asynchronously` when your application needs to choose between several resolved addresses.
This demo chooses the first address and binds a matching IPv4 or IPv6 local socket.

.. erbsland-demo::
    :source: network/TimeClient/TimeClientApp.hpp
    :function-blocks: onResolved onBound
    :function-blocks-sha256: cddceebb7d6449d4228042c72fd8dc50e3b67997df5a52d5e3e489aab248b652
    :source-sha256: 45c0300a8c6546798e7a35cab8e5f2609fd816951457c2ea10fd931257d43ddc

.. code-block:: cpp

    void onResolved(const el::List<el::IpAddress> &addresses) {
        if (addresses.isEmpty()) {
            throw el::ApplicationError{"The server host did not resolve to an IP address."_el};
        }

        _serverEndpoint = el::IpEndpoint{addresses.first(), _port};
        terminal()->printLine(
            el::cterm::fg::BrightBlack,
            "Creating a "_el,
            (_serverEndpoint.address().isV4() ? "IPv4"_el : "IPv6"_el),
            " socket"_el);

        _socket = events()->get<el::Network>().createUdpSocket();
        _socket->events()
            .onBound([this]() -> void { onBound(); })
            .onDatagram([this](const el::UdpDatagram &datagram) -> void { onDatagram(datagram); })
            .onClosed([this]() -> void { onClosed(); })
            .onError([this](const el::NetworkErrorContext &error) -> void { onError(error); });
        _state = State::Binding;
        _socket->start(_serverEndpoint.address().isV4() ? el::IpAddress::anyV4() : el::IpAddress::anyV6());
    }

    void onBound() {
        terminal()->printLine(fg::BrightGreen, "Bound to "_el, _socket->localEndpoint().value().toString());
        terminal()->printLine(fg::BrightBlack, "Sending time request to "_el, _serverEndpoint.toString());
        _state = State::WaitingForResponse;
        el::ByteBlock data;
        if (_protocol == 2) {
            data = el::ByteBlock{el::Byte{0x02U}};
        }
        if (!_socket->send(_serverEndpoint, data).isAccepted()) {
            throw el::ApplicationError{"The time request could not be queued."_el};
        }
        terminal()->printLine(fg::BrightBlack, "Waiting for response"_el);
        _timeout = events()->createTimer([this]() -> void { onTimeout(); });
        _timeout->startFixedDelay(el::TimeDelta::seconds(10));
    }

.. erbsland-demo-end::

Notice the ordering in the example: it waits for ``onBound()`` before sending the first packet.
At that point the local address family is established and the socket has a local endpoint.
The time protocol uses an empty request for version 1 and one byte for version 2, but any payload is represented by a
:cpp:class:`ByteBlock <erbsland::mem::ByteBlock>`.
When ``send()`` returns ``Accepted``, the socket keeps its own copy and the caller can release its copy.
That result means acceptance into the socket's output queue, not a delivery guarantee from UDP.

Handle Replies as Untrusted Packets
===================================

Treat every incoming datagram as untrusted, even when your application is waiting for one particular reply.
A UDP socket can receive packets from any reachable host, so the client verifies both the expected sender and the packet
length before interpreting the timestamp.
After accepting one valid reply, it closes the socket and waits for the asynchronous close notification before ending
the application.

.. erbsland-demo::
    :source: network/TimeClient/TimeClientApp.hpp
    :function-blocks: onDatagram onClosed
    :function-blocks-sha256: b9c99f10c40a34f1844e1892b0ff14e346710f8cd9c76bcc3d0af8e0e10ac0b2
    :source-sha256: 45c0300a8c6546798e7a35cab8e5f2609fd816951457c2ea10fd931257d43ddc

.. code-block:: cpp

    void onDatagram(const el::UdpDatagram &datagram) {
        if (_state != State::WaitingForResponse || datagram.remoteEndpoint() != _serverEndpoint) {
            return; // ignore random packets from unknown sources
        }
        const auto dataSize = datagram.data().length();
        if (dataSize != el::ByteLength{4} && dataSize != el::ByteLength{8}) {
            return; // ignore invalid lengths
        }
        el::Seconds ticks;
        if (dataSize == el::ByteLength{4}) {
            ticks = el::Seconds{
                static_cast<int64_t>(datagram.data().getInteger<std::uint32_t>(el::ByteIndex{}, el::Endianness::Big))};
        } else {
            ticks = el::Seconds{el::saturatingCast<int64_t>(
                datagram.data().getInteger<std::uint64_t>(el::ByteIndex{}, el::Endianness::Big))};
        }
        const auto time = el::DateTime::fromTicks(ticks, el::TimeEpoch::Rfc868).value_or(el::DateTime{});
        terminal()->printLine(fg::BrightGreen, "Received response from "_el, datagram.remoteEndpoint().toString());
        terminal()->printLine(fg::BrightWhite, "Time is "_el, time.toIsoString());
        _state = State::Closing;
        _socket->close();
    }

    void onClosed() {
        terminal()->printLine(fg::BrightBlack, "Closed socket"_el);
        quit();
    }

.. erbsland-demo-end::

The same checks protect a server protocol: validate packet data and sender information before allowing either to affect
application state.
UDP does not guarantee arrival order or uniqueness, so a protocol that needs those properties must carry request IDs,
sequence numbers, or its own retry rules.

React to a Full Output Queue
============================

Back-pressure is a normal part of sending UDP through an event loop.
``send()`` reports what happened to the local output queue; it does not report what happened on the network.
There are three immediate results.

.. list-table:: UDP send results
    :header-rows: 1
    :widths: 20 42 38

    *   - Result
        - Meaning
        - What to do
    *   - ``Accepted``
        - The socket retained the complete destination and payload.
        - The caller can release its copy.
    *   - ``WouldBlock``
        - The complete packet does not currently fit into the output queue.
        - Keep it unchanged and retry after ``onWritable()``.
    *   - ``Closed``
        - The socket cannot accept new output.
        - Stop sending and follow its terminal path.

The time server can safely skip a request when its reply cannot enter the queue because this small protocol is
stateless.
If your protocol cannot lose a packet, retain it in an application queue instead.
Remove the packet only after ``isAccepted()`` succeeds, and use ``onWritable()`` to retry the unchanged first packet.
The callback indicates that retrying may now succeed; always check the new ``send()`` result because other work may have
consumed the available capacity first.

``sendQueueLimit`` controls the amount of accepted output and defaults to one MiB.
An empty UDP packet is valid but consumes one byte for queue accounting.
A payload larger than ``maximumDatagramSize`` or ``sendQueueLimit`` can never fit and is rejected as an invalid argument
instead of returning ``WouldBlock``.

Pause Delivery Briefly
======================

Use ``pauseReceiving()`` only while code behind the packet callback cannot safely process input, for example while
replacing a protocol decoder or rebuilding a bounded downstream queue.
Pausing does not signal remote senders or reserve unlimited input.
The operating-system receive buffer can fill while delivery is paused, and UDP packets may then be lost.

The focused demo below pauses delivery while it replaces a catalog.
When delivery resumes, an oversized packet is reported as dropped and a valid packet still in the operating-system queue
is delivered.

.. erbsland-demo::
    :source: network/UdpSockets/PauseDelivery.cpp
    :function-blocks: beginReload sendUpdatesDuringReload finishReload reportDrop receiveUpdate
    :function-blocks-sha256: ab341b0028a7c83d9488c824b24918d50867e227550e662d39d4dfafaad78165
    :exec: network/udp_sockets --demo PauseDelivery
    :source-sha256: dfaec8d33b83bc1ae0ee10e21e569bc78583d8d23e2991f9c364fd0bdb94b4d6

.. code-block:: cpp

    void beginReload() {
        _service->pauseReceiving();
        el::io::printLine("Skill catalog reload started; delivery paused."_el);
        _client->start(el::IpAddress::loopbackV4());
    }

    void sendUpdatesDuringReload() {
        const auto destination = _service->localEndpoint().value();
        const auto oversized = el::ByteBlock{el::ByteLength{4U}, el::Byte{1U}};
        const auto valid = el::ByteBlock{el::ByteLength{1U}, el::Byte{2U}};
        if (!_client->send(destination, oversized).isAccepted() || !_client->send(destination, valid).isAccepted()) {
            el::stdErr()->printLine("The client could not queue both skill updates."_el);
            fail();
            return;
        }
        el::io::printLine("Client queued two updates while delivery was paused."_el);
        _resumeTimer->startOnce(el::Milliseconds{50});
    }

    void finishReload() {
        el::io::printLine("Skill catalog reload completed; delivery resumed."_el);
        _service->resumeReceiving();
    }

    void reportDrop(const el::UdpDatagramDropContext &drop) {
        if (drop.reason() == el::UdpDatagramDropReason::TooLarge) {
            el::io::printLine("Oversized skill update discarded."_el);
        }
    }

    void receiveUpdate(el::UdpDatagram datagram) {
        const auto id = datagram.data().get(el::ByteIndex{0U}).toUInt8();
        if (id == 2U) {
            el::io::printLine("Delivered after catalog reload: alchemie"_el);
        }
        _client->close();
        _service->close();
    }

.. erbsland-ansi::
    :escape-char: ␛

    Skill catalog reload started; delivery paused.
    Client queued two updates while delivery was paused.
    Skill catalog reload completed; delivery resumed.
    Oversized skill update discarded.
    Delivered after catalog reload: alchemie

.. erbsland-demo-end::

An incoming payload larger than ``maximumDatagramSize`` does not fail the socket.
Instead, ``onDatagramDropped()`` receives a
:cpp:class:`UdpDatagramDropContext <erbsland::network::UdpDatagramDropContext>` whose reason is ``TooLarge``.
The sender endpoint and observed packet size are optional because native platforms do not always report that information
for truncated packets.
Treat missing values as normal and use the information only when it is present.

Close the Socket Deliberately
=============================

Plan the socket's end state as deliberately as its start.
One :cpp:class:`UdpSocket <erbsland::network::UdpSocket>` represents one native socket lifetime and cannot be started
again after it closes or fails.
Call ``close()`` on the normal path, as the time client does: it stops new receives, drains output already accepted by
``send()``, and later emits ``onClosed()`` exactly once.
Call ``abort()`` only when cancellation must discard pending output and callbacks immediately; it is thread-safe and
does not emit ``onClosed()``.

All operations other than ``abort()`` belong to the owner event loop.
From another thread, use :cpp:func:`Events::invoke() <erbsland::event::Events::invoke>` to send, pause, resume, or close
the socket on that loop.

The essential pattern is small: bind the socket, wait for ``onBound()``, treat every datagram as untrusted, and decide
what your protocol should do when a packet cannot be queued or delivered.
Once those decisions are explicit, a UDP socket can remain a simple and reusable part of your event-driven design.
