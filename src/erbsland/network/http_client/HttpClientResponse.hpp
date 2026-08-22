// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HttpClientResponse_fwd.hpp"
#include "HttpClientResponseEventEditor.hpp"

#include "../http/HttpHeaders.hpp"
#include "../http/HttpResponseHead.hpp"
#include "../url/Url.hpp"

#include "../../event/EventSource.hpp"
#include "../../path/Path_fwd.hpp"
#include "../../stream/ByteOutputStream_fwd.hpp"
#include "../../unit/ByteLength.hpp"
#include "../../unit/ItemCount.hpp"

#include <utility>

namespace erbsland::network {

/// One retained final HTTP client response and its incoming body policy.
/// @seedoc{/reference/network/http_client}
/// @tested{HttpClientTest}
class HttpClientResponse : public event::EventSource {
public:
    // defaults
    ~HttpClientResponse() override = default;

public: // response data
    /// Get the immutable final response head.
    [[nodiscard]] virtual auto head() const noexcept -> const HttpResponseHead & = 0;
    /// Get completed trailer fields, or an empty collection before completion.
    [[nodiscard]] virtual auto trailers() const noexcept -> const HttpHeaders & = 0;
    /// Get the final effective URL after all followed redirects.
    [[nodiscard]] virtual auto effectiveUrl() const noexcept -> const Url & = 0;
    /// Get the number of redirects followed before this response.
    [[nodiscard]] virtual auto redirectCount() const noexcept -> unit::ItemCount = 0;

public: // incoming body policy
    /// Select incremental body delivery through `onBodyData`.
    virtual void streamBody() = 0;
    /// Select bounded byte aggregation through `onBody`.
    virtual void aggregateBody(unit::ByteLength maximumLength) = 0;
    /// Select bounded strict UTF-8 aggregation through `onText`.
    virtual void aggregateText(unit::ByteLength maximumLength) = 0;
    /// Select bounded JSON aggregation through `onJson`.
    virtual void aggregateJson(unit::ByteLength maximumLength) = 0;
    /// Pump body blocks into a byte output stream away from the event loop.
    virtual void writeBodyTo(stream::ByteOutputStreamPtr output) = 0;
    /// Atomically replace a destination path only after the complete response succeeds.
    virtual void writeBodyTo(path::Path destination) = 0;
    /// Reject the unread body and make the connection non-reusable.
    virtual void rejectBody() = 0;
    /// Pause an active streamed body.
    virtual void pauseBody() = 0;
    /// Resume an explicitly paused streamed body.
    virtual void resumeBody() = 0;

public: // lifecycle
    /// Test whether this response can no longer emit body events.
    [[nodiscard]] virtual auto isFinal() const noexcept -> bool = 0;
    /// Access the stable body callback editor.
    [[nodiscard]] auto events() -> HttpClientResponseEventEditor & override = 0;

protected:
    /// Create an abstract response owned by one event target.
    explicit HttpClientResponse(event::EventsPtr ownerEvents) : EventSource{std::move(ownerEvents)} {}
};

}
