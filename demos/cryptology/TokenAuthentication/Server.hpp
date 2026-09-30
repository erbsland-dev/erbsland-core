// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "JsonLine.hpp"

#include <erbsland/cryptology/authentication/PendingAuthenticationChallenge.hpp>
#include <erbsland/cryptology/authentication/ServerAuthenticationToken.hpp>
#include <erbsland/network/source/all.hpp>
#include <erbsland/network/tcp/all.hpp>

#include <algorithm>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

namespace demo {

/// One incoming TCP connection owns its challenge and accepts exactly one response.
/// A failed or malformed attempt ends this connection; another attempt needs a fresh challenge.
/// @notest{Compiled and exercised by the token authentication demo.}
class ServerSession final {
public:
    ServerSession(
        el::EventsPtr events,
        el::TcpConnectionRequestPtr request,
        el::cryptology::ServerAuthenticationToken token,
        std::function<void(ServerSession *)> done) :
        _events{std::move(events)}, _request{std::move(request)}, _token{std::move(token)}, _done{std::move(done)} {}

    void start() {
        _connection = _events->get<el::Network>().createTcpConnection();
        _connection->events()
            .onData([this](el::ByteBlock data) -> void { onData(std::move(data)); })
            .onError([this](const el::NetworkErrorContext &) -> void { _connection->abort(); })
            .onFinal([this]() -> void { _done(this); });
        auto options = el::TcpAcceptOptions{};
        options.setBufferLimits(el::SocketBufferLimits{el::ByteLength{8192U}, el::ByteLength{8192U}});
        _connection->accept(std::move(_request), options);
    }

private:
    void onData(el::ByteBlock data) {
        try {
            _lines.receive(data, [this](const el::json::JsonValue &record) -> void { onRecord(record); });
        } catch (const el::Exception &) {
            _connection->abort();
        }
    }

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

    void send(const el::json::JsonValue &record) {
        if (!_connection->send(JsonLine::encode(record)).isAccepted()) {
            throw el::ApplicationError{"Authentication record could not be queued."_el};
        }
    }

    enum class State { Hello, Proof, Finished };

private:
    el::EventsPtr _events;
    el::TcpConnectionRequestPtr _request;
    el::cryptology::ServerAuthenticationToken _token;
    std::function<void(ServerSession *)> _done;
    el::TcpConnectionPtr _connection;
    JsonLine _lines;
    std::optional<el::cryptology::PendingAuthenticationChallenge> _pending;
    State _state{State::Hello};
};

/// Listen on the configured endpoint and give every connection independent verification state.
/// @notest{Compiled and exercised by the token authentication demo.}
class Server final {
public:
    Server(
        el::EventsPtr events, el::IpAddress address, el::Port port, el::cryptology::ServerAuthenticationToken token) :
        _events{std::move(events)}, _address{std::move(address)}, _port{port}, _token{std::move(token)} {}

    void start() {
        _listener = _events->get<el::Network>().createTcpListener();
        _listener->events()
            .onListening([this]() -> void {
                el::io::printLine("Listening on "_el, _listener->localEndpoint().value().toString());
            })
            .onConnection([this](el::TcpConnectionRequestPtr request) -> void {
                auto session = std::make_shared<ServerSession>(
                    _events, std::move(request), _token, [this](ServerSession *finished) -> void {
                        removeSession(finished);
                    });
                _sessions.push_back(session);
                session->start();
            })
            .onError([](const el::NetworkErrorContext &error) -> void { throw el::NetworkError{error}; });
        _listener->start(el::IpEndpoint{_address, _port});
    }

private:
    void removeSession(ServerSession *finished) {
        _events->invoke([this, finished]() -> void {
            std::erase_if(_sessions, [finished](const auto &session) -> bool { return session.get() == finished; });
        });
    }

private:
    el::EventsPtr _events;
    el::IpAddress _address;
    el::Port _port;
    el::cryptology::ServerAuthenticationToken _token;
    el::TcpListenerPtr _listener;
    std::vector<std::shared_ptr<ServerSession>> _sessions;
};

}
