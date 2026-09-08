// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HttpRoutes.hpp"
#include "HttpServer_fwd.hpp"
#include "HttpServerConnection_fwd.hpp"
#include "HttpServerRequest_fwd.hpp"

#include "../codec/Http1DecodeEvent_fwd.hpp"
#include "../codec/Http1Transaction_fwd.hpp"
#include "../codec/Http1TransactionFailure_fwd.hpp"
#include "../codec/Http1TransactionOptions.hpp"
#include "../static/HttpStaticContent.hpp"

#include "../../../http/HttpMediaType.hpp"
#include "../../../http/HttpStatus.hpp"
#include "../../../http_server/HttpConnectionInfo.hpp"
#include "../../../http_server/HttpServerRequest_fwd.hpp"
#include "../../../http_server/HttpServerSession_fwd.hpp"
#include "../../../source/Connection_fwd.hpp"
#include "../../../source/NetworkErrorContext_fwd.hpp"
#include "../../../tcp/TcpConnectionRequest_fwd.hpp"

#include <cstddef>
#include <memory>
#include <optional>

namespace erbsland::network::impl {

/// One accepted connection orchestrated by an HTTP server.
/// @tested{HttpServerLiveTest}
class HttpServerConnection final : public std::enable_shared_from_this<HttpServerConnection> {
public:
    /// Retain one server and pending accepted TCP capability.
    HttpServerConnection(std::shared_ptr<HttpServer> server, TcpConnectionRequestPtr request);

public:
    /// Start plaintext adoption or TLS acceptance.
    void start();
    /// Disable transaction reuse and drain an active response.
    void serverClosing();
    /// Cancel setup or the active transaction immediately.
    void abort() noexcept;

private:
    /// Adopt the request as a plaintext TCP connection.
    void startPlain();
    /// Adopt the request as an authenticated TLS connection.
    void startTls();
    /// Validate active protocol state and begin HTTP.
    void handleActive();
    /// Create and bind one sequential HTTP/1 transaction.
    void startTransaction();
    /// Parse the request target and dispatch at the paused head checkpoint.
    void handleRequestHead(const Http1DecodeEvent &event);
    /// Translate an uncommitted transaction failure to a framework response.
    void handleFailure(const Http1TransactionFailure &failure);
    /// Capture the completed transaction persistence decision.
    void handleComplete(bool reusable);
    /// Finalize the public request and continue or close the connection.
    void handleFinal();
    /// Forward a setup transport error to the server.
    void handleSetupError(const NetworkErrorContext &context);
    /// Remove this connection from the server exactly once.
    void handleSetupFinal();
    /// Resolve the logical session and invoke the selected route.
    void dispatch(const std::shared_ptr<HttpServerRequest> &request, const Http1DecodeEvent &event);
    /// Validate and start automatic body handling or invoke a head handler.
    void invokeRoute(const HttpRouteHandler &handler, const Http1DecodeEvent &event);
    /// Convert and invoke one completed automatic request body.
    void handleAggregatedBody(mem::ByteBlock body);
    /// Invoke one application callback with framework exception handling.
    void invokeApplication(const std::function<void()> &callback);
    /// Test selected media type and charset policy for one automatic route.
    [[nodiscard]] auto acceptsContentType(const HttpRouteHandler &handler) const -> bool;
    /// Test one validated accepted media pattern.
    [[nodiscard]] static auto matchesContentTypePattern(const text::String &pattern, const HttpMediaType &mediaType)
        -> bool;
    /// Commit a bounded framework response when no public request exists.
    void sendFrameworkError(HttpStatus status);
    /// Translate captured server options into transaction options.
    [[nodiscard]] auto transactionOptions() const -> Http1TransactionOptions;
    /// Capture immutable public information from the current transport.
    [[nodiscard]] auto createConnectionInfo() const -> HttpConnectionInfo;
    /// Test whether decoded framing carries a semantic request body.
    [[nodiscard]] static auto hasBody(const Http1DecodeEvent &event) noexcept -> bool;
    /// Map a transaction failure phase to its bounded framework status.
    [[nodiscard]] static auto errorStatus(const Http1TransactionFailure &failure) noexcept -> HttpStatus;

private:
    std::weak_ptr<HttpServer> _server;                 ///< Owning server.
    TcpConnectionRequestPtr _acceptRequest;            ///< Pending accepted capability.
    ConnectionPtr _connection;                         ///< Active plaintext or TLS stream.
    std::optional<HttpConnectionInfo> _connectionInfo; ///< Immutable active-connection snapshot.
    Http1TransactionPtr _transaction;                  ///< Current sequential exchange.
    std::shared_ptr<HttpServerRequest> _request;       ///< Current retained public request.
    HttpRouteHandler _handler;                         ///< Current selected route handler.
    HttpStaticContentOperationPtr _staticContent;      ///< Active framework-owned static response.
    network::HttpServerSessionPtr _anonymousSession;   ///< Default per-connection session.
    std::size_t _requestCount{};                       ///< Started transaction count.
    bool _secure{};                                    ///< Whether this is an HTTPS connection.
    bool _closing{};                                   ///< Whether reuse is disabled.
    bool _reusable{};                                  ///< Last transaction result.
    bool _removed{};                                   ///< Server removal guard.
};

}
