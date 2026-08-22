// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HttpRequestTarget.hpp"
#include "HttpRoutes.hpp"
#include "HttpServerRequest_fwd.hpp"
#include "HttpServerRequestEventEditor_fwd.hpp"

#include "../codec/Http1Transaction_fwd.hpp"

#include "../../../http_server/HttpServerOptions.hpp"
#include "../../../http_server/HttpServerRequest.hpp"

#include <memory>

namespace erbsland::network::impl {

/// Built-in retained HTTP server request.
/// @tested{HttpServerLiveTest}
class HttpServerRequest final : public network::HttpServerRequest {
    friend class HttpServerRequestEventEditor;

public:
    /// Create one public request snapshot before session selection.
    HttpServerRequest(
        event::EventsPtr ownerEvents,
        HttpRequestHead head,
        HttpRequestTarget target,
        ConnectionPtr connection,
        HttpRoutes::Parameters parameters,
        HttpHeaders responseFields,
        HttpServerOptions options,
        bool hasBody,
        bool suppressBody);
    ~HttpServerRequest() override;

public: // implement network::HttpServerRequest
    /// Get the exact immutable request head.
    [[nodiscard]] auto head() const noexcept -> const HttpRequestHead & override;
    /// Get the decoded NFC path.
    [[nodiscard]] auto path() const noexcept -> const text::String & override;
    /// Get the exact query without its delimiter.
    [[nodiscard]] auto query() const noexcept -> const text::String & override;
    /// Get one route capture by unique name.
    [[nodiscard]] auto parameter(const text::String &name) const -> std::optional<text::String> override;
    /// Get the active connection local endpoint.
    [[nodiscard]] auto localEndpoint() const -> std::optional<IpEndpoint> override;
    /// Get the active connection remote endpoint.
    [[nodiscard]] auto remoteEndpoint() const -> std::optional<IpEndpoint> override;
    /// Get the concrete active connection.
    [[nodiscard]] auto connection() const noexcept -> const ConnectionPtr & override;
    /// Get the selected logical session.
    [[nodiscard]] auto session() const -> HttpServerSessionPtr override;
    /// Select streamed request-body delivery.
    void streamBody() override;
    /// Select bounded aggregated request-body delivery.
    void aggregateBody(unit::ByteLength maximumLength) override;
    /// Reject unread request body bytes.
    void rejectBody() override;
    /// Pause an active streamed body.
    void pauseBody() override;
    /// Resume an active streamed body.
    void resumeBody() override;
    /// Commit one bounded fixed response.
    void sendResponse(HttpResponseHead response, mem::ByteBlock body) override;
    /// Commit one UTF-8 plain-text response.
    void sendText(text::String body, HttpStatus status, HttpHeaders headers) override;
    /// Commit one UTF-8 JSON response.
    void sendJson(text::String body, HttpStatus status, HttpHeaders headers) override;
    /// Serialize and commit one compact JSON response.
    void sendJson(const text::json::JsonValue &body, HttpStatus status, HttpHeaders headers) override;
    /// Commit one plain-text error response.
    void sendError(HttpStatus status, text::String message) override;
    /// Commit one redirect response.
    void sendRedirect(text::String location, HttpStatus status, HttpHeaders headers) override;
    /// Commit one streamed response head.
    void startResponse(HttpResponseHead response) override;
    /// Atomically submit one streamed response block.
    [[nodiscard]] auto sendBody(const mem::ByteBlock &data) -> NetworkSendStatus override;
    /// Finish one streamed response with optional trailers.
    void finishBody(HttpHeaders trailers) override;
    /// Test whether the final response is committed.
    [[nodiscard]] auto isResponseStarted() const noexcept -> bool override;
    /// Test whether this request is safely stale.
    [[nodiscard]] auto isFinal() const noexcept -> bool override;
    /// Access the stable public request event editor.
    [[nodiscard]] auto events() -> network::HttpServerRequestEventEditor & override;

public: // internal orchestration
    /// Get pre-split decoded route segments.
    [[nodiscard]] auto segments() const noexcept -> const std::vector<text::String> & { return _target.segments(); }
    /// Test whether request framing carries a representation body.
    [[nodiscard]] auto hasBody() const noexcept -> bool { return _hasBody; }
    /// Attach the active transaction and provisional session.
    void attach(Http1TransactionPtr transaction, HttpServerSessionPtr session);
    /// Install manager selection, reserved cookie, and route captures.
    void setSelection(
        HttpServerSessionPtr session,
        HttpHeaders responseFields,
        std::optional<text::String> reservedCookieName,
        HttpRoutes::Parameters parameters);
    /// Deliver one streamed request-body block.
    void deliverBody(mem::ByteBlock data);
    /// Deliver one complete aggregated request body.
    void deliverAggregatedBody(mem::ByteBlock data);
    /// Deliver decoded request trailers.
    void deliverTrailers(const HttpHeaders &trailers);
    /// Deliver request-body completion.
    void deliverBodyCompleted();
    /// Deliver renewed semantic response capacity.
    void handleWritable();
    /// Make this retained request safely stale exactly once.
    void finalize();

private:
    /// Normalize a fixed response and append manager-owned fields.
    [[nodiscard]] auto prepareResponse(HttpResponseHead response, unit::ByteLength bodyLength) -> HttpResponseHead;
    /// Encode UTF-8 convenience output as a fixed response.
    void fixedResponse(HttpStatus status, HttpHeaders headers, mem::ByteBlock body, text::String contentType);
    /// Submit or retain the final framing operation.
    void queueFinish(HttpHeaders trailers);
    /// Append selection and invalidation fields to an application response.
    void appendManagerResponseFields(HttpHeaders &headers) const;
    /// Reject application use of a manager-reserved cookie name.
    void verifyReservedCookie(const HttpHeaders &headers) const;

private:
    HttpRequestHead _head;                                      ///< Exact request head.
    HttpRequestTarget _target;                                  ///< Parsed origin-form target.
    ConnectionPtr _connection;                                  ///< Concrete underlying connection.
    HttpServerSessionPtr _session;                              ///< Selected logical session.
    HttpRoutes::Parameters _parameters;                         ///< Route captures.
    HttpHeaders _responseFields;                                ///< Manager-provided response fields.
    std::optional<text::String> _reservedCookieName;            ///< Manager-owned response-cookie name.
    HttpServerOptions _options;                                 ///< Captured server limits.
    Http1TransactionPtr _transaction;                           ///< Active internal transaction.
    std::unique_ptr<HttpServerRequestEventEditor> _eventEditor; ///< Stable request editor.
    NetworkDataFn _onBodyData;                                  ///< Streamed body callback.
    NetworkDataFn _onBody;                                      ///< Aggregated body callback.
    std::function<void(const HttpHeaders &)> _onTrailers;       ///< Trailer callback.
    NetworkEventFn _onBodyCompleted;                            ///< Input completion callback.
    NetworkEventFn _onWritable;                                 ///< Semantic writable callback.
    NetworkEventFn _onFinal;                                    ///< Exactly-once final callback.
    std::optional<mem::ByteBlock> _pendingFixedBody;            ///< Internally retried fixed response body.
    std::optional<HttpHeaders> _pendingFinish;                  ///< Internally retried finish and trailers.
    bool _hasBody{};                                            ///< Whether the request head selected body framing.
    bool _suppressBody{};                                       ///< Whether HEAD suppresses response body bytes.
    bool _bodyPolicySelected{};                                 ///< Explicit or automatic body disposition.
    bool _responseStarted{};                                    ///< Final response commitment guard.
    bool _streamingResponse{};                                  ///< Whether public streamed body writes are valid.
    bool _finishRequested{};                                    ///< Final framing operation guard.
    bool _final{};                                              ///< Stale request guard.
};

}
