// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "Client.hpp"
#include "Server.hpp"

#include <erbsland/Application.hpp>
#include <erbsland/ApplicationError.hpp>
#include <erbsland/AuthenticationTokenPair.hpp>
#include <erbsland/ClientAuthenticationToken.hpp>
#include <erbsland/conf/Parser.hpp>
#include <erbsland/Host.hpp>
#include <erbsland/IpAddress.hpp>
#include <erbsland/Options.hpp>
#include <erbsland/OptionValues.hpp>
#include <erbsland/Path.hpp>
#include <erbsland/Port.hpp>
#include <erbsland/ServerAuthenticationToken.hpp>
#include <erbsland/StandardStreams.hpp>

#include <memory>

namespace demo {

/// Generate a token pair or run either side of the ELCL-configured TCP example.
/// @notest{Compiled and exercised by the token authentication demo.}
class TokenAuthenticationApp final : public el::Application {
public:
    using Application::Application;

protected: // implement Application
    void initialize() override { info().setApplicationName("Token Authentication Demo"_el); }

    void registerCommandLineOptions(const el::OptionsPtr &options) override {
        options->setHelpDescription("Generate a token pair or authenticate over JSONL/TCP."_el);
        options->addOption("mode"_el).setRequired().setHelpTitle("generate, server, or client"_el);
        options->addOption("value"_el).setRequired().setHelpTitle("identifier or ELCL configuration path"_el);
    }

    void handleCommandLineOptions(const el::OptionValuesPtr &values) override {
        const auto mode = values->getText("mode"_el);
        const auto value = values->getText("value"_el);
        if (mode == "generate"_el) {
            auto pair = el::cryptology::AuthenticationTokenPair::generate(value);
            el::io::printLine("Server token: "_el, pair.server().toString());
            el::io::printLine("Client token: "_el, pair.client().toString());
            loop()->invoke([this]() -> void { quit(); });
            return;
        }
        const auto document = el::conf::Parser{}.parseFileOrThrow(el::Path::fromNativeOrThrow(value));
        if (mode == "server"_el) {
            const auto address = el::IpAddress::fromStringOrThrow(document->getTextOrThrow("server.address"_el));
            const auto port = el::Port::fromStringOrThrow(document->getTextOrThrow("server.port"_el));
            const auto token = el::cryptology::ServerAuthenticationToken::fromStringOrThrow(
                document->getTextOrThrow("server.token"_el));
            _server = std::make_unique<Server>(loop(), address, port, token);
            loop()->invoke([this]() -> void { _server->start(); });
        } else if (mode == "client"_el) {
            const auto host = el::Host::fromStringOrThrow(document->getTextOrThrow("client.host"_el));
            const auto port = el::Port::fromStringOrThrow(document->getTextOrThrow("client.port"_el));
            auto tokenText = document->getTextOrThrow("client.token"_el);
            tokenText.markAsSensitive();
            auto token = el::cryptology::ClientAuthenticationToken::fromStringOrThrow(tokenText);
            _client = std::make_unique<Client>(loop(), host, port, std::move(token));
            loop()->invoke([this]() -> void { _client->start(); });
        } else {
            throw el::ApplicationError{"Mode must be generate, server, or client."_el};
        }
    }

private:
    std::unique_ptr<Server> _server;
    std::unique_ptr<Client> _client;
};

}

auto main(const int argc, char *argv[]) -> int {
    auto app = demo::TokenAuthenticationApp{argc, argv};
    return app.run();
}
