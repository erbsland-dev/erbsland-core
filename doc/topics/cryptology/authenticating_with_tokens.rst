..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    !single: Authentication Tokens
    single: Challenge Response Authentication
    single: JSON Lines; Token Authentication Demo

**************************
Authenticating with Tokens
**************************

Suppose a small service needs to recognize a configured client, but the client and server communicate over a TCP
connection that others can observe.
Sending a password or a reusable bearer token would let anyone who captures it sign in later.
An :cpp:class:`AuthenticationTokenPair <erbsland::cryptology::AuthenticationTokenPair>` gives the client a private
signing credential and the server a matching public verifier.
The client proves possession of its credential by signing a fresh challenge, without sending the credential itself.

This page follows a complete client and server that read separate ELCL files and exchange JSON Lines over TCP.
You can run the programs together, capture their wire traffic, and inspect every protocol message.
The example also makes the trust boundary visible: a successful challenge response identifies the holder of the client
key for this attempt, but it does not protect the rest of the connection.

What the Proof Establishes
==========================

The server chooses a fresh random nonce and remembers it in a
:cpp:class:`PendingAuthenticationChallenge <erbsland::cryptology::PendingAuthenticationChallenge>`.
The client signs the entire challenge with its private Ed25519 key.
The server checks the signature with the corresponding public key, checks that the signed challenge is exactly the one
it issued, and consumes the pending state on the first verification attempt.
A captured response therefore cannot authenticate a later attempt, even when it is sent by the same client.
The normal challenge lifetime is 60 seconds; the demo uses 30 seconds.

An observer of the TCP stream can read the identifier, challenge, response, and result.
Those values do not reveal the private credential.
An active intermediary can still relay a *live* challenge to the real client and relay its response to the server.
The proof does not authenticate the server to the client, bind the response to a particular TCP connection, encrypt
traffic, or authenticate messages sent after the proof.
Use a server-authenticated, end-to-end secure channel when those properties matter.
If the application continues after authentication, define how that channel or an additional message protocol binds later
requests to the verified connection.

Credentials and Configuration
=============================

Generate a pair once for each client identity.
The identifier is a public lookup value of one to 64 lowercase ASCII letters, digits, or underscores.
Choose a meaningful stable name such as ``site_001``; do not treat the visible name as proof that independently obtained
client and server token texts belong together.
When both are present during provisioning,
:cpp:func:`ServerAuthenticationToken::matches() <erbsland::cryptology::ServerAuthenticationToken::matches>` checks the
key relationship as well as the identifier.

After building the demo, generate a pair with:

.. code-block:: console

    cmake-build-debug/demo-apps/cryptology/token_authentication generate site_001

The command prints one ``Server token`` and one ``Client token``.
Run it in a private terminal and copy each complete value into its own ELCL file.
The ``-s-`` token contains the public verifier and can be installed on the server.
The ``-c-`` token contains a private seed: restrict access to the client file, do not log its value, and never put it in
a protocol message.
The two values share the same visible identifier so administrators can match their configuration files.

For a local run, create ``server.elcl`` and ``client.elcl`` as follows, replacing the token placeholders with the values
printed by the generator:

.. code-block:: elcl

    # server.elcl
    [server]
    Address: "127.0.0.1"
    Port: "45891"
    Token: "<Server token>"

.. code-block:: elcl

    # client.elcl
    [client]
    Host: "127.0.0.1"
    Port: "45891"
    Token: "<Client token>"

The application parses the configuration with :cpp:class:`Parser <erbsland::conf::Parser>` and imports each text value
using the role-specific ``fromStringOrThrow()`` method.
It marks the client token text as sensitive before importing the private key.
The server file needs only the public verifier; a server compromise alone cannot produce a valid client response.

.. erbsland-demo::
    :source: cryptology/TokenAuthentication/main.cpp
    :function-blocks: parseCommandLine
    :function-blocks-sha256: 41e0dc9697f0c351f0cd195af3abece50cf1dff387c47b48f78df47c398b90f1
    :source-sha256: 4edd7b069930049167be869d119859805c096e09db50cd62ad8060babda7c76f

.. code-block:: cpp

    void parseCommandLine() override {
        Application::parseCommandLine();
        if (optionValues() == nullptr) {
            return;
        }
        const auto mode = optionValues()->getText("mode"_el);
        const auto value = optionValues()->getText("value"_el);
        if (mode == "generate"_el) {
            auto pair = el::cryptology::AuthenticationTokenPair::generate(value);
            el::io::printLine("Server token: "_el, pair.server().toString());
            el::io::printLine("Client token: "_el, pair.client().toString());
            events()->invoke([this]() -> void { quit(); });
            return;
        }
        const auto document = el::conf::Parser{}.parseFileOrThrow(el::Path::fromNativeOrThrow(value));
        if (mode == "server"_el) {
            const auto address = el::IpAddress::fromStringOrThrow(document->getTextOrThrow("server.address"_el));
            const auto port = el::Port::fromStringOrThrow(document->getTextOrThrow("server.port"_el));
            const auto token = el::cryptology::ServerAuthenticationToken::fromStringOrThrow(
                document->getTextOrThrow("server.token"_el));
            _server = std::make_unique<Server>(events(), address, port, token);
            events()->invoke([this]() -> void { _server->start(); });
        } else if (mode == "client"_el) {
            const auto host = el::Host::fromStringOrThrow(document->getTextOrThrow("client.host"_el));
            const auto port = el::Port::fromStringOrThrow(document->getTextOrThrow("client.port"_el));
            auto tokenText = document->getTextOrThrow("client.token"_el);
            tokenText.markAsSensitive();
            auto token = el::cryptology::ClientAuthenticationToken::fromStringOrThrow(tokenText);
            _client = std::make_unique<Client>(events(), host, port, std::move(token));
            events()->invoke([this]() -> void { _client->start(); });
        } else {
            throw el::ApplicationError{"Mode must be generate, server, or client."_el};
        }
    }

.. erbsland-demo-end::

The Exchange on the Wire
========================

The client sends a ``hello`` record containing only its public identifier.
The server uses that identifier to select a verifier, creates a fresh challenge, and sends the challenge bytes as
canonical, unpadded Base64url.
The client checks that the binary challenge addresses its own identifier before signing it.
It returns a serialized signed response in the same text encoding.
The server verifies the response once and replies with an acceptance result.

.. code-block:: text

    client -> server  {"type":"hello","id":"site_001"}
    server -> client  {"type":"challenge","data":"<Base64url challenge>"}
    client -> server  {"type":"response","data":"<Base64url signed response>"}
    server -> client  {"type":"result","accepted":true}

Each JSON object is followed by one LF byte.
TCP can split or combine writes, so the demo collects bytes until it sees a complete line; it never assumes one socket
callback equals one message.
It limits an incoming line to 4096 bytes, parses one JSON object, and rejects malformed Base64url.
JSON makes a packet capture easy to read while Base64url preserves the exact binary challenge and signed response.
The JSON wrapper is only transport framing; it is not part of the signed data.

.. erbsland-demo::
    :source: cryptology/TokenAuthentication/JsonLine.hpp
    :function-blocks: receive
    :function-blocks-sha256: 95100519be0342216c9c23348de6ab31630e6bd1c5c0b5b95e03f15c6b52a00b
    :source-sha256: 071dc566acd0a762a038e5bf6fe221129ecf5880f94adba8d00a6daca9c5aa27

.. code-block:: cpp

    void receive(const el::ByteBlock &data, Fn &&onLine) {
        for (const auto byte : data.span()) {
            if (byte == el::Byte{'\n'}) {
                if (_buffer.isEmpty()) {
                    throw el::ApplicationError{"Empty authentication record."_el};
                }
                const auto source = el::StringDecoder{_buffer}.decode(
                    el::StringEncoding::Utf8, el::StringBomMode::Reject, el::EncodingMode::Strict);
                _buffer.clear();
                auto limits = el::json::JsonParseOptions{};
                limits.setMaximumInputLength(el::ByteLength{4096U});
                const auto value = el::json::JsonValue::fromStringOrThrow(source, limits);
                if (!value.is(el::json::JsonType::Object)) {
                    throw el::ApplicationError{"Authentication record must be a JSON object."_el};
                }
                onLine(value);
            } else {
                if (_buffer.length() >= el::ByteLength{4096U} || byte == el::Byte{'\r'}) {
                    throw el::ApplicationError{"Authentication record is too long or contains CR."_el};
                }
                _buffer.append(byte);
            }
        }
    }

.. erbsland-demo-end::

Serving One Authentication Attempt
==================================

The server stores a public :cpp:class:`ServerAuthenticationToken <erbsland::cryptology::ServerAuthenticationToken>`.
In this compact demo it has one configured client, so a ``hello`` ID must match that token.
A service with several clients would look up the verifier by ID in its own registry and still keep the selected verifier
and pending challenge with the individual connection.

Each accepted TCP connection gets a separate ``ServerSession``.
After ``hello``, the session calls ``createChallenge()`` and keeps the returned pending state until it receives one
``response``.
It decodes the response and calls ``verify()`` once; a failure, malformed frame, or disconnect ends that attempt.
An expired response is rejected, while a production listener should also close connections that stay idle.
The ``result`` message is the last application message, so this example grants no access to a later request that would
need separate protection.

.. erbsland-demo::
    :source: cryptology/TokenAuthentication/Server.hpp
    :function-blocks: onRecord
    :function-blocks-sha256: 3e16673af4f7bbc7cfc02cd761e2c10c166b6628992228b3338b35c2428355fa
    :source-sha256: e3b0f4966ed6c75c889399e87b8ac1179233a9e185c24df51349e14a75cba43d

.. code-block:: cpp

    void onRecord(const el::json::JsonValue &record) {
        if (_state == State::Hello) {
            if (record.getOrThrow("type"_el).getTextOrThrow() != "hello"_el ||
                record.getOrThrow("id"_el).getTextOrThrow() != _token.id()) {
                throw el::ApplicationError{"Unknown authentication identifier."_el};
            }
            // Keep the one-use pending state with this TCP session.
            _pending = _token.createChallenge(el::time::Seconds{30});
            auto answer = el::json::JsonValue{el::json::JsonObject{}};
            answer.set("type"_el, el::String{"challenge"_el});
            answer.set("data"_el, JsonLine::base64(_pending->challengeBytes()));
            send(answer);
            _state = State::Proof;
            return;
        }
        if (_state != State::Proof || record.getOrThrow("type"_el).getTextOrThrow() != "response"_el) {
            throw el::ApplicationError{"Unexpected authentication record."_el};
        }
        const auto response = JsonLine::unbase64(record.getOrThrow("data"_el).getTextOrThrow(), el::ByteLength{497U});
        const auto accepted = _pending->verify(response.span());
        _pending.reset();
        _state = State::Finished;
        auto answer = el::json::JsonValue{el::json::JsonObject{}};
        answer.set("type"_el, el::String{"result"_el});
        answer.set("accepted"_el, accepted);
        send(answer);
        // The demo grants no application access beyond this result.
        _connection->close();
    }

.. erbsland-demo-end::

Answering the Challenge
=======================

The client loads a :cpp:class:`ClientAuthenticationToken <erbsland::cryptology::ClientAuthenticationToken>` from its
protected ELCL file.
After sending its public ID, it decodes the received challenge and passes the exact bytes to ``createResponse()``.
That call checks the challenge structure and identifier before signing, then returns a
:cpp:class:`SignedByteBlock <erbsland::cryptology::SignedByteBlock>`.
The client sends only its serialized signed response.

The client treats a missing or negative result as failure.
In an application that needs to trust a server's answer, the application must protect that answer with an authenticated
channel; an unauthenticated TCP peer can forge ``accepted`` or close the stream.

.. erbsland-demo::
    :source: cryptology/TokenAuthentication/Client.hpp
    :function-blocks: sendHello onRecord
    :function-blocks-sha256: 44b24a937c108a03ba4404957483d3ffd9871fc7cc66122106127254cb0925a0
    :source-sha256: a3414373895a16ebc8e08dece9d71ee2a855339e653b3c3c0230d44e70b38e05

.. code-block:: cpp

    void sendHello() {
        auto hello = el::json::JsonValue{el::json::JsonObject{}};
        hello.set("type"_el, el::String{"hello"_el});
        hello.set("id"_el, _token.id());
        send(hello);
    }

    void onRecord(const el::json::JsonValue &record) {
        if (_state == State::Challenge) {
            if (record.getOrThrow("type"_el).getTextOrThrow() != "challenge"_el) {
                throw el::ApplicationError{"Expected an authentication challenge."_el};
            }
            const auto challenge =
                JsonLine::unbase64(record.getOrThrow("data"_el).getTextOrThrow(), el::ByteLength{102U});
            // createResponse checks that the challenge addresses this token's identifier.
            const auto response = _token.createResponse(challenge.span()).toByteBlock();
            auto answer = el::json::JsonValue{el::json::JsonObject{}};
            answer.set("type"_el, el::String{"response"_el});
            answer.set("data"_el, JsonLine::base64(response));
            send(answer);
            _state = State::Result;
            return;
        }
        if (_state != State::Result || record.getOrThrow("type"_el).getTextOrThrow() != "result"_el) {
            throw el::ApplicationError{"Unexpected authentication record."_el};
        }
        _accepted = record.getOrThrow("accepted"_el).getBoolOrThrow();
        _receivedResult = true;
        _state = State::Finished;
        el::io::printLine(_accepted ? "Authentication accepted."_el : "Authentication rejected."_el);
        _connection->close();
    }

.. erbsland-demo-end::

Run the server in one terminal and the client in another:

.. code-block:: console

    cmake-build-debug/demo-apps/cryptology/token_authentication server server.elcl
    cmake-build-debug/demo-apps/cryptology/token_authentication client client.elcl

The client prints ``Authentication accepted.`` and exits successfully when the configured tokens match.
Because the transport is plain TCP, a capture of this local exchange shows all four JSONL records, including the public
ID and binary values encoded as text, but no ``elctk-...-c-...`` credential.
On macOS, for example, ``sudo tcpdump -A -i lo0 'tcp port 45891'`` captures the local exchange while the client runs.
For a real deployment, restrict who can reach the listener and use a secure channel where server identity, message
integrity, or confidentiality is required.

Token and Response Format
=========================

The JSONL exchange has two layers.
JSON supplies message boundaries and carries binary values as Base64url text; the authentication rules operate on the
decoded bytes.
If you implement a client or server in another language, reproduce the byte formats below before wrapping them in JSON.
The :doc:`signed byte block topic <signing_and_verifying_byte_blocks>` explains the shared response format in full.

Credential Text
---------------

The two credentials have parallel text shapes:

.. code-block:: text

    client credential:  elctk-<id>-c-<data>
    server credential:  elctk-<id>-s-<data>

    <id>    public identifier
    c / s   role: private client seed / public server key
    <data>  Base64url of 34 binary bytes

The public ``<id>`` contains one to 64 lowercase ASCII letters, digits, or underscores: ``[_a-z0-9]{1,64}``.
It is a lookup name, so matching text alone does not prove that two tokens contain a related key pair.
The ``-c-`` and ``-s-`` role markers are literal parts of the token text; they are not among the encoded bytes.

Decode ``<data>`` as canonical Base64url with no ``=`` padding or whitespace.
It represents exactly 34 bytes:

.. code-block:: text

    byte 0       0x01              token format version
    byte 1       0x01              Ed25519 algorithm
    bytes 2..33  32-byte value     private seed for c; public key for s

Generate the private seed with a cryptographically secure random source and derive its public key using standard
Ed25519. Install the ``c`` token only on the client and the ``s`` token on the server.
A parser rejects a wrong role, version, algorithm, length, invalid key, or noncanonical encoding rather than trying to
repair it.

Challenge Bytes
---------------

After a ``hello`` names a client, the server creates a new challenge for that attempt.
The challenge is a compact binary message, which the JSONL ``challenge.data`` field carries as unpadded Base64url:

.. code-block:: text

    ASCII "ELCA" | 0x01 | ID length | ASCII ID | 32 random nonce bytes
       4 bytes     1 byte    1 byte     1..64           32 bytes

The ID must be the same public identifier used for this credential.
Thus the binary challenge is 39 to 102 bytes long, depending on ID length.
The client validates the magic, version, exact length, identifier syntax, and matching ID before it signs anything.

The server retains the *exact challenge bytes* and a local monotonic deadline with this authentication attempt.
The nonce makes another attempt different even if the same ID is used.
The deadline is server state; it is not a timestamp supplied by the client and is not embedded in the challenge.
When the response arrives, the server compares its verified payload byte for byte with the challenge it retained and
consumes the attempt, including on failure.

Response Bytes
--------------

The client signs the complete challenge as a
:cpp:class:`SignedByteBlock <erbsland::cryptology::SignedByteBlock>`.
Its ``response.data`` JSON field is unpadded Base64url of that binary record.
At a high level, the decoded record has this layout:

.. code-block:: text

    ELSB header | client ID | "erbsland.core.auth-token.challenge.v1" |
                | exact ELCA challenge bytes | 64-byte Ed25519 signature

The ELSB header contains version ``0x01``, algorithm ``0x01`` for Ed25519, byte lengths for the ID and purpose, and a
four-byte big-endian payload length.
The signed key-ID field holds the client's public ID; the purpose is exactly ``erbsland.core.auth-token.challenge.v1``
in UTF-8; and the payload is the complete challenge, including its ``ELCA`` header and nonce.
The signature covers the entire ELSB record before the signature, not merely the nonce or payload.
See :doc:`signing_and_verifying_byte_blocks` for field offsets, canonical parsing rules, and the signature input.

The server uses the ID only to locate a candidate public key.
It accepts the proof only when the Ed25519 signature verifies with that key, the expected purpose matches, the signed ID
matches the selected client, and the verified payload equals its still-pending challenge.
Only then can the server treat the response as proof of possession for this attempt.
The JSON wrapper, including its ``type`` and ``data`` field names, is not signed; it is transport framing around the
binary proof.
