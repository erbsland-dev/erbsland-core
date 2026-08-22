// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HttpServerRequest_fwd.hpp"
#include "HttpServerRequestEventEditor.hpp"
#include "HttpServerSession_fwd.hpp"

#include "../http/HttpHeaders.hpp"
#include "../http/HttpRequestHead.hpp"
#include "../http/HttpResponseHead.hpp"
#include "../IpEndpoint.hpp"
#include "../source/Connection_fwd.hpp"
#include "../source/NetworkSendStatus.hpp"

#include "../../event/EventSource.hpp"
#include "../../mem/ByteBlock.hpp"
#include "../../text/json/JsonValue.hpp"
#include "../../text/String.hpp"
#include "../../unit/ByteLength.hpp"

#include <optional>
#include <utility>

namespace erbsland::network {

/// One retained request and response transaction on an HTTP server.
/// @tested{HttpServerLiveTest}
class HttpServerRequest : public event::EventSource {
public:
    // defaults
    ~HttpServerRequest() override = default;

public: // request data
    /// Get the immutable exact request head.
    [[nodiscard]] virtual auto head() const noexcept -> const HttpRequestHead & = 0;
    /// Get the decoded NFC path without its query.
    [[nodiscard]] virtual auto path() const noexcept -> const text::String & = 0;
    /// Get the exact query text without the question mark.
    [[nodiscard]] virtual auto query() const noexcept -> const text::String & = 0;
    /// Get a captured complete-segment or catch-all route parameter.
    [[nodiscard]] virtual auto parameter(const text::String &name) const -> std::optional<text::String> = 0;
    /// Get the concrete connection's local endpoint.
    [[nodiscard]] virtual auto localEndpoint() const -> std::optional<IpEndpoint> = 0;
    /// Get the concrete connection's remote endpoint.
    [[nodiscard]] virtual auto remoteEndpoint() const -> std::optional<IpEndpoint> = 0;
    /// Get the concrete plaintext or TLS connection.
    [[nodiscard]] virtual auto connection() const noexcept -> const ConnectionPtr & = 0;
    /// Get the selected logical session.
    [[nodiscard]] virtual auto session() const -> HttpServerSessionPtr = 0;

public: // incoming body
    /// Select incremental body delivery through ``onBodyData``.
    virtual void streamBody() = 0;
    /// Select atomic aggregation under a finite request-specific maximum.
    virtual void aggregateBody(unit::ByteLength maximumLength) = 0;
    /// Reject unread body bytes and make the connection non-reusable.
    virtual void rejectBody() = 0;
    /// Pause an active streamed body.
    virtual void pauseBody() = 0;
    /// Resume an explicitly paused streamed body.
    virtual void resumeBody() = 0;

public: // response
    /// Commit one fixed response and optional body atomically.
    virtual void sendResponse(HttpResponseHead response, mem::ByteBlock body = {}) = 0;
    /// Commit one UTF-8 plain-text response.
    virtual void sendText(text::String body, HttpStatus status = HttpStatus::Ok, HttpHeaders headers = {}) = 0;
    /// Commit one UTF-8 JSON response.
    virtual void sendJson(text::String body, HttpStatus status = HttpStatus::Ok, HttpHeaders headers = {}) = 0;
    /// Serialize and commit one compact JSON response.
    virtual void sendJson(
        const text::json::JsonValue &body, HttpStatus status = HttpStatus::Ok, HttpHeaders headers = {}) = 0;
    /// Commit one plain-text error response.
    virtual void sendError(HttpStatus status, text::String message = {}) = 0;
    /// Commit one redirect with a Location field.
    virtual void sendRedirect(
        text::String location, HttpStatus status = HttpStatus::Found, HttpHeaders headers = {}) = 0;
    /// Commit the head of one streamed response.
    virtual void startResponse(HttpResponseHead response) = 0;
    /// Atomically submit one streamed response-body block.
    [[nodiscard]] virtual auto sendBody(const mem::ByteBlock &data) -> NetworkSendStatus = 0;
    /// Finish a streamed response with optional trailers.
    virtual void finishBody(HttpHeaders trailers = {}) = 0;
    /// Test whether the final response head is committed.
    [[nodiscard]] virtual auto isResponseStarted() const noexcept -> bool = 0;
    /// Test whether this retained request can no longer affect its connection.
    [[nodiscard]] virtual auto isFinal() const noexcept -> bool = 0;
    /// Access the stable body, writable, and final callback editor.
    [[nodiscard]] auto events() -> HttpServerRequestEventEditor & override = 0;

protected:
    /// Create an abstract retained request owned by one event target.
    explicit HttpServerRequest(event::EventsPtr ownerEvents) : EventSource{std::move(ownerEvents)} {}
};

}
