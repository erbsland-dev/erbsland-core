// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HttpClientRedirectOptions.hpp"
#include "HttpClientRequest_fwd.hpp"
#include "HttpClientSession_fwd.hpp"
#include "HttpClientSessionEventEditor.hpp"
#include "HttpClientSessionOptions.hpp"
#include "HttpClientTlsOptions.hpp"
#include "HttpCookieJar.hpp"

#include "../http/HttpHeaders.hpp"
#include "../http/HttpMethod.hpp"
#include "../source/NetworkSourceState.hpp"
#include "../url/Url.hpp"

#include "../../event/EventSource.hpp"
#include "../../mem/ByteBlock.hpp"

#include <utility>

namespace erbsland::network {

/// A session-first HTTP/1.1 and HTTPS client.
/// @seedoc{/reference/network/http_client}
/// @tested{HttpClientTest NetworkFacadeTest}
class HttpClientSession : public event::EventSource {
public:
    // defaults
    ~HttpClientSession() override = default;

public: // configuration
    /// Replace limits and deadlines used by subsequently submitted requests.
    virtual void setOptions(HttpClientSessionOptions options) = 0;
    /// Replace curated HTTPS options used by subsequently submitted requests.
    virtual void setTlsOptions(HttpClientTlsOptions options) = 0;
    /// Replace the redirect policy captured by subsequently submitted requests.
    virtual void setRedirectOptions(HttpClientRedirectOptions options) = 0;
    /// Replace default fields used by subsequently submitted requests.
    virtual void setDefaultHeaders(HttpHeaders headers) = 0;
    /// Access the stable session-owned cookie jar.
    [[nodiscard]] virtual auto cookieJar() noexcept -> HttpCookieJar & = 0;

public: // requests
    /// Create one prepared GET request.
    [[nodiscard]] virtual auto createRequest(Url url) -> HttpClientRequestPtr = 0;
    /// Create one prepared request.
    [[nodiscard]] virtual auto createRequest(HttpMethod method, Url url) -> HttpClientRequestPtr = 0;
    /// Submit a prepared request created by this session.
    virtual void sendRequest(HttpClientRequestPtr request) = 0;
    /// Create and submit a GET request.
    [[nodiscard]] virtual auto sendGet(Url url) -> HttpClientRequestPtr = 0;
    /// Create and submit a HEAD request.
    [[nodiscard]] virtual auto sendHead(Url url) -> HttpClientRequestPtr = 0;
    /// Create and submit a fixed-body POST request.
    [[nodiscard]] virtual auto sendPost(Url url, mem::ByteBlock body, HttpHeaders headers = {})
        -> HttpClientRequestPtr = 0;

public: // lifecycle
    /// Get the session lifecycle state.
    [[nodiscard]] virtual auto state() const noexcept -> NetworkSourceState = 0;
    /// Stop admission and drain every submitted request.
    virtual void close() = 0;
    /// Cancel every queued or active request immediately.
    virtual void abort() noexcept = 0;
    /// Access the stable callback and default response-policy editor.
    [[nodiscard]] auto events() -> HttpClientSessionEventEditor & override = 0;

protected:
    /// Create an abstract session owned by one event target.
    explicit HttpClientSession(event::EventsPtr ownerEvents) : EventSource{std::move(ownerEvents)} {}
};

}
