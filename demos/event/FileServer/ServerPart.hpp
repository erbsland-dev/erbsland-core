// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/all_core.hpp>
#include <erbsland/all_event.hpp>
#include <erbsland/all_mem.hpp>
#include <erbsland/all_network.hpp>
#include <erbsland/all_network_http.hpp>
#include <erbsland/all_network_http_server.hpp>
#include <erbsland/all_network_source.hpp>
#include <erbsland/all_options.hpp>
#include <erbsland/all_path.hpp>
#include <erbsland/all_stream.hpp>
#include <erbsland/all_text.hpp>
#include <erbsland/all_time.hpp>
#include <erbsland/ApplicationPart.hpp>
#include <erbsland/ApplicationPartIdentifier.hpp>
#include <erbsland/ApplicationPartState.hpp>
#include <erbsland/ByteBlock.hpp>
#include <erbsland/HttpFieldType.hpp>
#include <erbsland/HttpHeaders.hpp>
#include <erbsland/HttpMethod.hpp>
#include <erbsland/HttpMethodType.hpp>
#include <erbsland/HttpResponseHead.hpp>
#include <erbsland/HttpServer.hpp>
#include <erbsland/HttpServerRequest.hpp>
#include <erbsland/HttpServerSession.hpp>
#include <erbsland/HttpStaticFileHandler.hpp>
#include <erbsland/HttpStatus.hpp>
#include <erbsland/HttpVersion.hpp>
#include <erbsland/IpAddress.hpp>
#include <erbsland/IpEndpoint.hpp>
#include <erbsland/Network.hpp>
#include <erbsland/NetworkError.hpp>
#include <erbsland/NetworkErrorContext.hpp>
#include <erbsland/NetworkSourceState.hpp>
#include <erbsland/Options.hpp>
#include <erbsland/OptionValues.hpp>
#include <erbsland/Path.hpp>
#include <erbsland/Port.hpp>
#include <erbsland/StandardStreams.hpp>
#include <erbsland/String.hpp>
#include <erbsland/StringBomMode.hpp>
#include <erbsland/StringEncoder.hpp>
#include <erbsland/StringEncoding.hpp>
#include <erbsland/TimeDelta.hpp>

#include <memory>

namespace demo {

using namespace el::text::literals;

/// Own an HTTP server on an application part's dedicated event thread.
/// Command-line hooks configure the part before startup; lifecycle hooks create and close its event sources.
/// @notest{Verified by running the file_server demo.}
class ServerPart final : public el::ApplicationPart {
public: // registration
    /// Get the stable identifier used to register this part.
    [[nodiscard]] static auto partIdentifier() -> el::ApplicationPartIdentifierPtr {
        static const auto identifier = el::ApplicationPartIdentifier::create("dev.erbsland.demo.file-server"_el);
        return identifier;
    }
    /// Construct this part when the application prepares its registered parts.
    /// @return The new server part.
    [[nodiscard]] static auto create() -> std::shared_ptr<ServerPart> { return std::make_shared<ServerPart>(); }

public: // implement ApplicationPart
    void registerCommandLineOptions(const el::OptionsPtr &options) override;
    void parseCommandLine(const el::OptionValuesPtr &values) override;

protected: // implement ApplicationPart
    void initialize() override {
        // The network source belongs to this part's event loop.
        _server = events()->get<el::Network>().createHttpServer();
        _server->addStaticContentHandler(el::HttpStaticFileHandler::create(_root, "/files"_el));
        _server->events()
            .onRequest(
                el::HttpMethod{el::HttpMethodType::Get},
                "/"_el,
                [this](el::HttpServerSessionPtr, el::HttpServerRequestPtr request, el::ByteBlock) -> void {
                    auto headers = el::HttpHeaders{};
                    headers.setField(el::HttpFieldType::ContentType, "text/html; charset=utf-8"_el);
                    const auto body = createIndex();
                    request->sendResponse(
                        el::HttpResponseHead{el::HttpVersion::Http11, el::HttpStatus::Ok, "OK"_el, headers},
                        el::StringEncoder{body}.encode(el::StringEncoding::Utf8, el::StringBomMode::Reject));
                })
            .onListening([this]() -> void {
                el::io::printLine("Browse http://"_el, _server->localEndpoint()->toString(), "/"_el);
                if (_timeout.isPositive()) {
                    events()->invokeAfter(_timeout, []() -> void { el::application().quit(); });
                }
            })
            .onError([](const el::NetworkErrorContext &error) -> void { throw el::NetworkError{error}; })
            .onFinal([this]() -> void {
                if (state() == el::ApplicationPartState::Stopping) {
                    completeStopping();
                }
            });
        // Bind to loopback so the demo exposes files only on the local machine.
        _server->start(el::IpEndpoint{el::IpAddress::fromStringOrThrow("127.0.0.1"_el), _port});
    }
    void stopping() override {
        // Allow committed responses to drain before ending this part's event loop.
        if (_server == nullptr || _server->state() == el::NetworkSourceState::Closed ||
            _server->state() == el::NetworkSourceState::Failed) {
            completeStopping();
        } else {
            _server->close();
        }
    }
    void cleanup() noexcept override {
        if (_server != nullptr) {
            _server->abort();
            _server.reset();
        }
    }

private:
    /// Build the HTML listing for the configured content root.
    /// @return The HTML document with links to regular files.
    auto createIndex() const -> el::String;

private:
    el::Path _root;            ///< Configured, physically resolved content root.
    el::Port _port;            ///< Configured loopback listening port.
    el::TimeDelta _timeout;    ///< Optional demo run duration.
    el::HttpServerPtr _server; ///< Event source owned by this part's loop.
};

}
