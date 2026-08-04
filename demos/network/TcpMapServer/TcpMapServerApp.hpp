// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/all.hpp>
#include <erbsland/conf/Parser.hpp>
#include <erbsland/cterm/Terminal.hpp>
#include <erbsland/network/source/all.hpp>
#include <erbsland/network/tcp/all.hpp>

#include <functional>
#include <memory>
#include <utility>
#include <vector>

namespace demo {

using namespace el::text::literals;
using namespace el::cterm;

using Map = el::StringMap<el::String>;

/// Serve byte-writer-framed map queries over TCP.
/// Each accepted connection owns a session that reassembles the byte stream before looking up one map key.
/// @notest{Compiled and exercised as part of the TCP map server demo.}
class MapSession final {
public:
    using DoneFn = std::function<void(MapSession *)>;
    using RequestFn = std::function<void(const el::IpEndpoint &, const el::String &)>;

    MapSession(
        el::EventsPtr events,
        std::shared_ptr<const Map> map,
        el::TcpConnectionRequestPtr request,
        RequestFn requestFn,
        DoneFn done) :
        _events{std::move(events)},
        _map{std::move(map)},
        _request{std::move(request)},
        _requestFn{std::move(requestFn)},
        _done{std::move(done)} {}

    void start() {
        _connection = _events->get<el::Network>().createTcpConnection();
        _connection->events()
            .onData([this](el::ByteBlock data) -> void { onData(std::move(data)); })
            .onError([](const el::NetworkErrorContext &error) -> void { throw el::NetworkError{error}; })
            .onFinal([this]() -> void { _done(this); });
        auto options = el::TcpAcceptOptions{};
        options.setBufferLimits(el::SocketBufferLimits{el::ByteLength{8192U}, el::ByteLength{8192U}});
        _connection->accept(std::move(_request), options);
    }

private:
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

    [[nodiscard]] static auto textOptions() -> el::ByteTextOptions {
        return el::ByteTextOptions{}.setLength(el::ByteLength{4096U});
    }

private:
    el::EventsPtr _events;
    std::shared_ptr<const Map> _map;
    el::TcpConnectionRequestPtr _request;
    RequestFn _requestFn;
    DoneFn _done;
    el::TcpConnectionPtr _connection;
    el::ByteBlockEditor _input;
};

/// A standalone TCP map server with one session object per accepted connection.
/// @notest{Compiled and exercised as part of the TCP map server demo.}
class TcpMapServerApp final : public el::Application {
public:
    using Application::Application;

protected: // implement Application
    void initialize() override { info().setApplicationName("TCP Map Server"_el); }

    void registerCommandLineOptions(const el::OptionsPtr &options) override {
        options->setHelpDescription("Serves text values from an ELCL map over TCP."_el);
        options->addOption("map-file"_el)
            .setRequired()
            .setHelpDescription("ELCL document containing the map section."_el);
        options->addOption({"-a"_el, "--address"_el, "address"_el})
            .setType(el::OptionType::Text)
            .setDefaultValue("127.0.0.1"_el)
            .setHelpDescription("Local IP address to bind."_el)
            .setValidateTextValue<el::IpAddress>("The address is not a valid IP address"_el);
        options->addOption({"-p"_el, "--port"_el, "port"_el})
            .setType(el::OptionType::Text)
            .setDefaultValue("45870"_el)
            .setHelpDescription("Local TCP port to bind."_el)
            .setValidateTextValue<el::Port>("The port is not valid"_el);
    }

    void parseCommandLine() override {
        Application::parseCommandLine();
        if (optionValues() == nullptr) {
            return;
        }
        _address = el::IpAddress::fromStringOrThrow(optionValues()->getText("address"_el));
        _port = el::Port::fromStringOrThrow(optionValues()->getText("port"_el));
        _map = loadMap(el::Path::fromNativeOrThrow(optionValues()->getText("map-file"_el)));
        events()->invoke([this]() -> void { startServer(); });
    }

private:
    auto loadMap(const el::Path &path) -> std::shared_ptr<const Map> {
        const auto document = el::conf::Parser{}.parseFileOrThrow(path);
        const auto mapSection = document->getSectionWithTextsOrThrow("map"_el);
        auto map = Map{};
        for (const auto &entry : *mapSection) {
            map.set(entry->name().asText(), entry->asTextOrThrow());
        }
        return std::make_shared<Map>(std::move(map));
    }

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

private:
    el::IpAddress _address;
    el::Port _port;
    std::shared_ptr<const Map> _map;
    el::TcpListenerPtr _listener;
    std::vector<std::shared_ptr<MapSession>> _sessions;
};

}
