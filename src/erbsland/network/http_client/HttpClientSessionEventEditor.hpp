// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HttpClientRequestFn.hpp"
#include "HttpClientResponseOptions.hpp"

#include "../source/NetworkEventFn.hpp"

#include "../../event/impl/CommonEventEditor.hpp"

#include <utility>

namespace erbsland::network {

/// Callback and default response-policy editor for an HTTP client session.
/// @tested{HttpClientTest}
class HttpClientSessionEventEditor : public event::impl::CommonEventEditor {
public:
    // defaults
    ~HttpClientSessionEventEditor() override = default;

public:
    /// Replace the default bounded byte-response handler.
    virtual auto onResponse(HttpClientResponseFn callback, HttpClientResponseOptions options = {})
        -> HttpClientSessionEventEditor & = 0;
    /// Replace the default bounded strict UTF-8 response handler.
    virtual auto onTextResponse(HttpClientTextResponseFn callback, HttpClientResponseOptions options = {})
        -> HttpClientSessionEventEditor & = 0;
    /// Replace the default bounded JSON response handler.
    virtual auto onJsonResponse(HttpClientJsonResponseFn callback, HttpClientResponseOptions options = {})
        -> HttpClientSessionEventEditor & = 0;
    /// Replace the default low-level final-response-head handler.
    virtual auto onResponseHead(HttpClientResponseHeadFn callback) -> HttpClientSessionEventEditor & = 0;
    /// Replace the informational-response handler.
    virtual auto onInformationalResponse(HttpClientInformationalResponseFn callback)
        -> HttpClientSessionEventEditor & = 0;
    /// Replace the redirect-policy fallback handler.
    virtual auto onRedirect(HttpClientRedirectFn callback) -> HttpClientSessionEventEditor & = 0;
    /// Replace the request-error fallback handler.
    virtual auto onError(HttpClientErrorFn callback) -> HttpClientSessionEventEditor & = 0;
    /// Replace the graceful session-closure handler.
    virtual auto onClosed(NetworkEventFn callback) -> HttpClientSessionEventEditor & = 0;
    /// Replace the exactly-once session final handler.
    virtual auto onFinal(NetworkEventFn callback) -> HttpClientSessionEventEditor & = 0;

protected:
    /// Create an abstract session editor retaining its source and callback target.
    HttpClientSessionEventEditor(event::EventSourcePtr source, event::EventsPtr target) :
        CommonEventEditor{std::move(source), std::move(target)} {}
};

}
