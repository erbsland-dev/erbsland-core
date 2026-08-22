// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HttpRoutes.hpp"
#include "HttpServer_fwd.hpp"
#include "HttpServerConnection_fwd.hpp"
#include "HttpServerEventEditor_fwd.hpp"

#include "../static/HttpStaticContent.hpp"

#include "../../../http_server/HttpServer.hpp"
#include "../../../tcp/TcpConnectionRequest_fwd.hpp"
#include "../../../tcp/TcpListener_fwd.hpp"
#include "../../../tls/TlsServerAcceptOptions.hpp"

#include <memory>
#include <optional>
#include <vector>

namespace erbsland::network::impl {

/// Built-in HTTP/1.1 and HTTPS server.
/// @tested{HttpServerLiveTest}
class HttpServer final : public network::HttpServer {
    friend class HttpServerConnection;
    friend class HttpServerEventEditor;
    friend class HttpStaticContentOperation;

public:
    /// Create an inactive server owned by one event loop.
    explicit HttpServer(event::EventsPtr ownerEvents);
    ~HttpServer() override;

public: // implement network::HttpServer
    /// Replace HTTP limits while inactive.
    void setOptions(HttpServerOptions options) override;
    /// Replace listener options while inactive.
    void setListenerOptions(TcpListenerOptions options) override;
    /// Replace accepted TCP stream options while inactive.
    void setTcpAcceptOptions(TcpAcceptOptions options) override;
    /// Enable HTTPS using the default curated options while inactive.
    void enableTls() override;
    /// Enable HTTPS using curated options while inactive.
    void enableTls(HttpServerTlsOptions options) override;
    /// Replace the synchronous session manager while inactive.
    void setSessionManager(HttpServerSessionManagerPtr manager) override;
    /// Add one static-content handler while inactive.
    void addStaticContentHandler(HttpStaticContentHandlerPtr handler) override;
    /// Get the effective listener endpoint.
    [[nodiscard]] auto localEndpoint() const -> std::optional<IpEndpoint> override;
    /// Get the server lifecycle state.
    [[nodiscard]] auto state() const noexcept -> NetworkSourceState override;
    /// Validate configuration and bind the internal listener.
    void start(IpEndpoint localEndpoint) override;
    /// Pause internal listener acceptance.
    void pauseAccepting() override;
    /// Resume internal listener acceptance.
    void resumeAccepting() override;
    /// Gracefully drain accepted connections.
    void close() override;
    /// Abort listener and connections immediately.
    void abort() noexcept override;
    /// Access the stable public server editor.
    [[nodiscard]] auto events() -> network::HttpServerEventEditor & override;

private:
    /// Adopt one listener-emitted TCP request.
    void handleConnection(TcpConnectionRequestPtr request);
    /// Enter failed state from one listener error.
    void handleListenerError(const NetworkErrorContext &context);
    /// Remove one finalized accepted connection.
    void removeConnection(const std::shared_ptr<HttpServerConnection> &connection);
    /// Create and announce one server-owned logical session.
    [[nodiscard]] auto createSession(std::optional<text::String> identifier, HttpSessionDataPtr data)
        -> network::HttpServerSessionPtr;
    /// Ask the manager for invalidation response fields.
    void sessionInvalidated(const network::HttpServerSessionPtr &session);
    /// Emit graceful closure after listener and connections drain.
    void assessClosed();
    /// Require the configurable inactive state.
    void verifyInactive() const;
    /// Validate finite HTTP and curated HTTPS limits.
    void validateOptions() const;
    /// Derive the private low-level TLS accept options for this server start.
    [[nodiscard]] auto createTlsAcceptOptions() const -> TlsServerAcceptOptions;
    /// Reserve one active static-content response.
    [[nodiscard]] auto reserveStaticResponse() noexcept -> bool;
    /// Release one active static-content response.
    void releaseStaticResponse() noexcept;
    /// Reserve one pending blocking operation.
    [[nodiscard]] auto reserveStaticOperation() noexcept -> bool;
    /// Release one pending blocking operation.
    void releaseStaticOperation() noexcept;
    /// Reserve completed bytes awaiting output.
    [[nodiscard]] auto reserveStaticQueue(unit::ByteLength length) noexcept -> bool;
    /// Release completed bytes awaiting output.
    void releaseStaticQueue(unit::ByteLength length) noexcept;
    /// Reserve retained static-content memory.
    [[nodiscard]] auto reserveStaticMemory(unit::ByteLength length) noexcept -> bool;
    /// Release retained static-content memory.
    void releaseStaticMemory(unit::ByteLength length) noexcept;

private:
    std::unique_ptr<HttpServerEventEditor> _eventEditor;             ///< Stable server editor.
    network::TcpListenerPtr _listener;                               ///< Owned native listener.
    std::vector<std::shared_ptr<HttpServerConnection>> _connections; ///< Active accepted connections.
    HttpRoutes _routes;                                              ///< Server-level routes.
    HttpServerOptions _options;                                      ///< HTTP limits and deadlines.
    TcpListenerOptions _listenerOptions;                             ///< Listener policy.
    TcpAcceptOptions _tcpAcceptOptions;                              ///< Plain accepted-stream options.
    std::optional<HttpServerTlsOptions> _tlsOptions;                 ///< Optional curated HTTPS policy.
    std::optional<TlsServerAcceptOptions> _tlsAcceptOptions;         ///< Derived private HTTPS policy.
    HttpServerSessionManagerPtr _sessionManager;                     ///< Optional custom logical-session selector.
    std::vector<HttpStaticContentHandlerPtr> _staticContentHandlers; ///< Configured static-content handlers.
    std::vector<HttpStaticContentHandlerPtr> _staticContent;         ///< Frozen priority-ordered original handlers.
    HttpStaticContentUsePtr _staticContentUse;                       ///< Shared configuration-freeze lease.
    NetworkEventFn _onListening;                                     ///< Listening-ready callback.
    HttpServerSessionFn _onNewSession;                               ///< New logical-session callback.
    NetworkEventFn _onClosed;                                        ///< Graceful closure callback.
    NetworkErrorFn _onError;                                         ///< Operational error callback.
    NetworkEventFn _onFinal;                                         ///< Exactly-once final callback.
    NetworkSourceState _state{NetworkSourceState::Inactive};         ///< Server lifecycle.
    bool _listenerFinal{};                                           ///< Whether listener finalization completed.
    bool _finalEmitted{};                                            ///< Exactly-once server final guard.
    std::size_t _activeStaticResponses{};                            ///< Active static-content responses.
    std::size_t _activeStaticOperations{};                           ///< Pending/in-flight blocking operations.
    std::uint64_t _staticQueueLength{};                              ///< Bytes awaiting output.
    std::uint64_t _staticMemoryLength{};                             ///< Retained static-content memory bytes.
};

}
