// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HttpClientRedirectOptions.hpp"
#include "HttpClientRequest_fwd.hpp"
#include "HttpClientRequestEventEditor.hpp"

#include "../http/HttpHeaders.hpp"
#include "../http/HttpMethod.hpp"
#include "../source/NetworkSendStatus.hpp"
#include "../source/NetworkSourceState.hpp"
#include "../url/Url.hpp"

#include "../../event/EventSource.hpp"
#include "../../mem/ByteBlock.hpp"

#include <utility>

namespace erbsland::network {

/// One prepared or submitted HTTP client request.
/// @seedoc{/reference/network/http_client}
/// @tested{HttpClientTest}
class HttpClientRequest : public event::EventSource {
public:
    // defaults
    ~HttpClientRequest() override = default;

public: // immutable request data
    /// Get the request method.
    [[nodiscard]] virtual auto method() const noexcept -> const HttpMethod & = 0;
    /// Get the absolute request URL.
    [[nodiscard]] virtual auto url() const noexcept -> const Url & = 0;

public: // preparation
    /// Get the request-specific fields.
    [[nodiscard]] virtual auto headers() const noexcept -> const HttpHeaders & = 0;
    /// Replace request-specific fields while prepared.
    virtual void setHeaders(HttpHeaders headers) = 0;
    /// Select a fixed request body while prepared.
    virtual void setBody(mem::ByteBlock body) = 0;
    /// Select streamed request-body framing while prepared.
    virtual void streamBody() = 0;
    /// Override the captured session redirect policy while prepared.
    virtual void setRedirectOptions(HttpClientRedirectOptions options) = 0;

public: // streamed upload
    /// Atomically submit one streamed request-body block.
    [[nodiscard]] virtual auto sendBody(const mem::ByteBlock &data) -> NetworkSendStatus = 0;
    /// Finish a streamed request body with optional trailers.
    [[nodiscard]] virtual auto finishBody(HttpHeaders trailers = {}) -> NetworkSendStatus = 0;

public: // lifecycle
    /// Get the request lifecycle state.
    [[nodiscard]] virtual auto state() const noexcept -> NetworkSourceState = 0;
    /// Cancel this request without affecting sibling requests.
    virtual void cancel() noexcept = 0;
    /// Access the stable request callback and response-policy editor.
    [[nodiscard]] auto events() -> HttpClientRequestEventEditor & override = 0;

protected:
    /// Create an abstract request owned by one event target.
    explicit HttpClientRequest(event::EventsPtr ownerEvents) : EventSource{std::move(ownerEvents)} {}
};

}
