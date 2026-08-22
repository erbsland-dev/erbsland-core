// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/all.hpp>
#include <erbsland/cterm/Terminal.hpp>
#include <erbsland/network/source/all.hpp>
#include <erbsland/network/tcp/all.hpp>

#include <utility>

namespace demo {

using namespace el::text::literals;
using namespace el::cterm;

/// Connect to a separately running map server and exchange one byte-writer-framed query.
/// Incoming TCP chunks are accumulated until the complete response frame is available.
/// @notest{Compiled and exercised as part of the TCP map client demo.}
class TcpMapClientApp final : public el::Application {
public:
    using Application::Application;

protected: // implement Application
    void initialize() override { info().setApplicationName("TCP Map Client"_el); }

    void registerCommandLineOptions(const el::OptionsPtr &options) override {
        options->setHelpDescription("Looks up one text key on a TCP map server."_el);
        options->addOption("key"_el).setRequired().setHelpDescription("Text key to look up."_el);
        options->addOption({"--host"_el, "host"_el})
            .setType(el::OptionType::Text)
            .setDefaultValue("127.0.0.1"_el)
            .setHelpDescription("Map server host name or IP address."_el)
            .setValidateTextValue<el::Host>("The host is not a valid host name or IP address"_el);
        options->addOption({"-p"_el, "--port"_el, "port"_el})
            .setType(el::OptionType::Text)
            .setDefaultValue("45870"_el)
            .setHelpDescription("Map server TCP port."_el)
            .setValidateTextValue<el::Port>("The port is not valid"_el);
    }

    void parseCommandLine() override {
        Application::parseCommandLine();
        if (optionValues() == nullptr) {
            return;
        }
        _key = optionValues()->getText("key"_el);
        _host = el::Host::fromStringOrThrow(optionValues()->getText("host"_el));
        _port = el::Port::fromStringOrThrow(optionValues()->getText("port"_el));
        events()->invoke([this]() -> void { startClient(); });
    }

private:
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

    [[nodiscard]] static auto textOptions() -> el::ByteTextOptions {
        return el::ByteTextOptions{}.setLength(el::ByteLength{4096U});
    }

private:
    el::String _key;
    el::Host _host;
    el::Port _port;
    el::TcpConnectionPtr _connection;
    el::ByteBlockEditor _input;
    bool _receivedAnswer{false};
};

}
