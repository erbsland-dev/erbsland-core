// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "HttpsServerApp.hpp"

#include <erbsland/cryptology/configuration/CryptologyConfiguration.hpp>
#include <erbsland/cryptology/keys/SigningPrivateKey.hpp>
#include <erbsland/cryptology/tls/TlsConfiguration.hpp>
#include <erbsland/cryptology/tls/TlsServerIdentity.hpp>
#include <erbsland/cryptology/x509/X509CertificateBundle.hpp>
#include <erbsland/network/source/NetworkError.hpp>

#include <algorithm>
#include <cstdint>

namespace demo {

using namespace el::text::literals;

void HttpsServerApp::initialize() {
    info().setApplicationName("HTTPS Server Demo"_el);
}

void HttpsServerApp::registerCommandLineOptions(const el::OptionsPtr &options) {
    options->setHelpDescription(
        "Runs an HTTPS server with routes, static overlays, streamed output, and graceful shutdown."_el);
    options->addOption({"-a"_el, "--address"_el, "address"_el})
        .setType(el::OptionType::Text)
        .setDefaultValue("127.0.0.1"_el)
        .setValidateTextValue<el::IpAddress>("The address is not a valid IP address"_el)
        .setHelpDescription("Local IP address to bind."_el);
    options->addOption({"-p"_el, "--port"_el, "port"_el})
        .setType(el::OptionType::Text)
        .setDefaultValue("8443"_el)
        .setValidateTextValue<el::Port>("The port is not valid"_el)
        .setHelpDescription("Local HTTPS port to bind; zero selects an ephemeral port."_el);
    options->addOption({"--certificate"_el, "certificate"_el})
        .setType(el::OptionType::Text)
        .setRequired()
        .setValueName("file"_el)
        .setHelpDescription("PEM certificate chain presented by the server."_el);
    options->addOption({"--private-key"_el, "private-key"_el})
        .setType(el::OptionType::Text)
        .setRequired()
        .setValueName("file"_el)
        .setHelpDescription("Unencrypted PKCS#8 PEM private key matching the leaf certificate."_el);
    options->addOption({"--static-root"_el, "static-root"_el})
        .setType(el::OptionType::Text)
        .setRequired()
        .setMaximum(el::ArgumentCount{16U})
        .setValueName("directory"_el)
        .setHelpDescription("Directory mounted at /assets; repeat it in highest-to-lowest overlay priority."_el);
    options->addOption({"--run-for"_el, "run-for"_el})
        .setType(el::OptionType::Integer)
        .setDefaultValue(el::OptionInteger{0})
        .setValueName("seconds"_el)
        .setHelpDescription("Initiate graceful shutdown after this many seconds; zero runs indefinitely."_el);
}

void HttpsServerApp::parseCommandLine() {
    Application::parseCommandLine();
    if (optionValues() == nullptr) {
        return;
    }
    _address = el::IpAddress::fromStringOrThrow(optionValues()->getText("address"_el));
    _port = el::Port::fromStringOrThrow(optionValues()->getText("port"_el));
    _certificatePath = el::Path::fromNativeOrThrow(optionValues()->getText("certificate"_el));
    _privateKeyPath = el::Path::fromNativeOrThrow(optionValues()->getText("private-key"_el));
    for (const auto &root : optionValues()->getTextList("static-root"_el)) {
        _staticRoots.emplace_back(el::Path::fromNativeOrThrow(root));
    }
    const auto runFor = optionValues()->getInteger("run-for"_el);
    if (runFor < 0) {
        throw el::ApplicationError{"The run duration cannot be negative."_el};
    }
    _runFor = el::Seconds{runFor};
    configureTls();
    events()->invoke([this]() -> void { startServer(); });
}

/// Register the certificate and matching private key under the HTTP server's standard TLS configuration label.
void HttpsServerApp::configureTls() {
    auto configuration = el::cryptology::TlsConfiguration{};
    configuration.setServerIdentity(
        el::cryptology::TlsServerIdentity{
            el::cryptology::X509CertificateBundle::fromFileOrThrow(_certificatePath),
            el::cryptology::SigningPrivateKey::fromPemOrThrow(_privateKeyPath.content().readTextOrThrow())});
    cryptologyConfiguration().setTlsConfiguration("http/server"_el, std::move(configuration));
}

/// Mount repeated static roots at one URL prefix, preserving command-line order as overlay priority.
void HttpsServerApp::configureStaticContent() {
    auto priority = std::int32_t{static_cast<std::int32_t>(_staticRoots.size())};
    for (const auto &root : _staticRoots) {
        auto handler = el::HttpStaticFileHandler::create(root, "/assets"_el);
        handler->setPriority(priority--);
        _server->addStaticContentHandler(std::move(handler));
    }
}

/// Register fixed, parameterized, and streamed routes through the HTTP server event editor.
void HttpsServerApp::configureRoutes() {
    _server->events()
        .onRequest(
            el::HttpMethod{el::HttpMethodType::Get},
            "/health"_el,
            [](el::HttpServerSessionPtr, el::HttpServerRequestPtr request, el::ByteBlock) -> void {
                request->sendJson("{\"status\":\"ok\"}"_el);
            })
        .onRequest(
            el::HttpMethod{el::HttpMethodType::Get},
            "/hello/{name}"_el,
            [](el::HttpServerSessionPtr, el::HttpServerRequestPtr request, el::ByteBlock) -> void {
                request->sendText(
                    el::StringFormat{"Hello, {}!"_el}.build(request->parameter("name"_el).value_or("visitor"_el)));
            })
        .onRequestHead(
            el::HttpMethod{el::HttpMethodType::Get},
            "/stream"_el,
            [this](el::HttpServerSessionPtr, el::HttpServerRequestPtr request) -> void {
                auto response = std::make_shared<StreamedResponse>(
                    std::move(request), [this](StreamedResponse *finished) -> void { removeResponse(finished); });
                _responses.emplace_back(response);
                response->start();
            });
}

/// Create the HTTPS server, attach lifecycle callbacks, and bind its configured endpoint.
void HttpsServerApp::startServer() {
    _server = events()->get<el::Network>().createHttpServer();
    _server->enableTls();
    configureStaticContent();
    configureRoutes();
    _server->events()
        .onListening([this]() -> void { onListening(); })
        .onClosed([]() -> void { el::stdOut()->printLine("HTTPS server closed gracefully."_el); })
        .onError([](const el::NetworkErrorContext &error) -> void { throw el::network::NetworkError{error}; })
        .onFinal([this]() -> void { quit(); });
    _server->start(el::IpEndpoint{_address, _port});
}

void HttpsServerApp::onListening() {
    el::stdOut()->printLine("HTTPS server listening on "_el, _server->localEndpoint()->toString());
    el::stdOut()->printLine("Routes: /health, /hello/{name}, /stream, /assets/..."_el);
    if (_runFor.isPositive()) {
        events()->invokeAfter(_runFor, [this]() -> void { closeGracefully(); });
    }
}

/// Stop accepting new connections and let active HTTPS responses drain before quitting the application.
void HttpsServerApp::closeGracefully() {
    if (_closing) {
        return;
    }
    _closing = true;
    el::stdOut()->printLine("Initiating graceful shutdown..."_el);
    _server->close();
}

void HttpsServerApp::removeResponse(StreamedResponse *finished) {
    events()->invoke([this, finished]() -> void {
        std::erase_if(_responses, [finished](const auto &response) -> bool { return response.get() == finished; });
    });
}

}
