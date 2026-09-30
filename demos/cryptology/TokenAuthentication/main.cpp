// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "Client.hpp"
#include "Server.hpp"

#include <erbsland/conf/Parser.hpp>
#include <erbsland/cryptology/authentication/AuthenticationTokenPair.hpp>

#include <memory>

namespace demo {

/// Generate a token pair or run either side of the ELCL-configured TCP example.
/// @notest{Compiled and exercised by the token authentication demo.}
class TokenAuthenticationApp final : public el::Application {
public:
    using Application::Application;

protected:
    void initialize() override { info().setApplicationName("Token Authentication Demo"_el); }

    void registerCommandLineOptions(const el::OptionsPtr &options) override {
        options->setHelpDescription("Generate a token pair or authenticate over JSONL/TCP."_el);
        options->addOption("mode"_el).setRequired().setHelpDescription("generate, server, or client"_el);
        options->addOption("value"_el).setRequired().setHelpDescription("identifier or ELCL configuration path"_el);
    }

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

private:
    std::unique_ptr<Server> _server;
    std::unique_ptr<Client> _client;
};

}

auto main(const int argc, char *argv[]) -> int {
    auto app = demo::TokenAuthenticationApp{argc, argv};
    return app.run();
}
