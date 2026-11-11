// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ServerStatus.hpp"

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

#include <cstddef>
#include <memory>
#include <mutex>

namespace demo {

using namespace el::text::literals;

/// Own an HTTP server on an application part's dedicated event thread.
/// Command-line hooks configure the part before startup; lifecycle hooks create and close its event sources.
class ServerPart final : public el::ApplicationPartWithInterface<ServerStatus> {
public: // registration
    /// Construct this part when the application prepares its registered parts.
    /// @return The new server part.
    [[nodiscard]] static auto create() -> std::shared_ptr<ServerPart> { return std::make_shared<ServerPart>(); }

public: // implement ServerStatus
    [[nodiscard]] auto indexRequestCount() const -> std::size_t override;

public: // implement ApplicationPart
    void registerCommandLineOptions(const el::OptionsPtr &options) override;
    void handleCommandLineOptions(const el::OptionValuesPtr &values) override;

protected: // implement ApplicationPart
    void onInitialize() override {
        // The network source belongs to this part's event loop.
        _server = loop()->get<el::Network>().createHttpServer();
        _server->addStaticContentHandler(el::HttpStaticFileHandler::create(_root, "/files"_el));
        // Handle directory-index requests on the part thread.
        _server->events().onRequest(
            el::HttpMethod{el::HttpMethodType::Get},
            "/"_el,
            [this](el::HttpServerSessionPtr, el::HttpServerRequestPtr request, el::ByteBlock) -> void {
                recordIndexRequest();
                auto headers = el::HttpHeaders{};
                headers.setField(el::HttpFieldType::ContentType, "text/html; charset=utf-8"_el);
                const auto body = createIndex();
                request->sendResponse(
                    el::HttpResponseHead{el::HttpVersion::Http11, el::HttpStatus::Ok, "OK"_el, headers},
                    el::StringEncoder{body}.encode(el::StringEncoding::Utf8, el::StringBomMode::Reject));
            });
        // Report listener readiness after the endpoint has been bound.
        _server->events().onListening([this]() -> void {
            el::io::printLine("Browse http://"_el, _server->localEndpoint()->toString(), "/"_el);
            if (_timeout.isPositive()) {
                loop()->invokeAfter(_timeout, []() -> void { el::application().quit(); });
            }
        });
        // Escalate source errors to the part manager.
        _server->events().onError([](const el::NetworkErrorContext &error) -> void { throw el::NetworkError{error}; });
        // Complete part shutdown after the source has closed.
        _server->events().onFinal([this]() -> void {
            if (state() == el::ApplicationPartState::Stopping) {
                completeStopping();
            }
        });
        // Bind to loopback so the demo exposes files only on the local machine.
        _server->start(el::IpEndpoint{el::IpAddress::fromStringOrThrow("127.0.0.1"_el), _port});
    }
    void onStopping() override {
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
    /// Record a directory-index request from the HTTP callback on the part thread.
    void recordIndexRequest();
    /// Build the HTML listing for the configured content root.
    /// @return The HTML document with links to regular files.
    auto createIndex() const -> el::String;

private:
    el::Path _root;                      ///< Configured, physically resolved content root.
    el::Port _port;                      ///< Configured loopback listening port.
    el::TimeDelta _timeout;              ///< Optional demo run duration.
    el::HttpServerPtr _server;           ///< Event source owned by this part's loop.
    mutable std::mutex _statisticsMutex; ///< Protects the count for readers on other threads.
    std::size_t _indexRequestCount{0};   ///< Directory-index requests received on the part loop.
};

}
