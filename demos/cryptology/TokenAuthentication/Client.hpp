// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "JsonLine.hpp"

#include <erbsland/cryptology/authentication/ClientAuthenticationToken.hpp>
#include <erbsland/cryptology/SignedByteBlock.hpp>
#include <erbsland/network/source/all.hpp>
#include <erbsland/network/tcp/all.hpp>

namespace demo {

/// Prove possession of a client credential using a challenge from the configured TCP server.
/// The secret token text and private key never enter a JSON record.
/// @notest{Compiled and exercised by the token authentication demo.}
class Client final {
public:
    Client(el::EventsPtr events, el::Host host, el::Port port, el::cryptology::ClientAuthenticationToken token) :
        _events{std::move(events)}, _host{std::move(host)}, _port{port}, _token{std::move(token)} {}

    void start() {
        _connection = _events->get<el::Network>().createTcpConnection();
        _connection->events()
            .onConnected([this]() -> void { sendHello(); })
            .onData([this](el::ByteBlock data) -> void { onData(std::move(data)); })
            .onClosed([this](const el::ConnectionCloseContext &) -> void {
                if (!_receivedResult) {
                    el::io::printLine("Server closed without an authentication result."_el);
                    el::application().quit(el::ExitCode::failure());
                }
            })
            .onError([](const el::NetworkErrorContext &) -> void { el::application().quit(el::ExitCode::failure()); })
            .onFinal([this]() -> void {
                el::application().quit(_accepted ? el::ExitCode::success() : el::ExitCode::failure());
            });
        _connection->connect(el::HostEndpoint{_host, _port});
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

    void sendHello() {
        auto hello = el::json::JsonValue{el::json::JsonObject{}};
        hello.set("type"_el, el::String{"hello"_el});
        hello.set("id"_el, _token.id());
        send(hello);
    }

    void send(const el::json::JsonValue &record) {
        if (!_connection->send(JsonLine::encode(record)).isAccepted()) {
            throw el::ApplicationError{"Authentication record could not be queued."_el};
        }
    }

    enum class State { Challenge, Result, Finished };

private:
    el::EventsPtr _events;
    el::Host _host;
    el::Port _port;
    el::cryptology::ClientAuthenticationToken _token;
    el::TcpConnectionPtr _connection;
    JsonLine _lines;
    State _state{State::Challenge};
    bool _receivedResult{false};
    bool _accepted{false};
};

}
