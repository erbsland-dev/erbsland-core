// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HttpClientRequestFn.hpp"
#include "HttpClientResponseOptions.hpp"

#include "../source/NetworkEventFn.hpp"

#include "../../event/impl/CommonEventEditor.hpp"

#include <utility>

namespace erbsland::network {

/// Callback and response-policy editor for one HTTP client request.
/// @tested{HttpClientTest}
class HttpClientRequestEventEditor : public event::impl::CommonEventEditor {
public:
    // defaults
    ~HttpClientRequestEventEditor() override = default;

public:
    /// Select and replace bounded byte-response handling for this request.
    virtual auto onResponse(HttpClientResponseFn callback, HttpClientResponseOptions options = {})
        -> HttpClientRequestEventEditor & = 0;
    /// Select and replace bounded strict UTF-8 response handling for this request.
    virtual auto onTextResponse(HttpClientTextResponseFn callback, HttpClientResponseOptions options = {})
        -> HttpClientRequestEventEditor & = 0;
    /// Select and replace bounded JSON response handling for this request.
    virtual auto onJsonResponse(HttpClientJsonResponseFn callback, HttpClientResponseOptions options = {})
        -> HttpClientRequestEventEditor & = 0;
    /// Select and replace low-level final-response-head handling for this request.
    virtual auto onResponseHead(HttpClientResponseHeadFn callback) -> HttpClientRequestEventEditor & = 0;
    /// Replace the informational-response handler.
    virtual auto onInformationalResponse(HttpClientInformationalResponseFn callback)
        -> HttpClientRequestEventEditor & = 0;
    /// Replace the redirect-policy handler.
    virtual auto onRedirect(HttpClientRedirectFn callback) -> HttpClientRequestEventEditor & = 0;
    /// Replace the initial and renewed upload-writable handler.
    virtual auto onWritable(NetworkEventFn callback) -> HttpClientRequestEventEditor & = 0;
    /// Replace the operational-error handler.
    virtual auto onError(HttpClientErrorFn callback) -> HttpClientRequestEventEditor & = 0;
    /// Replace the exactly-once request final handler.
    virtual auto onFinal(NetworkEventFn callback) -> HttpClientRequestEventEditor & = 0;

protected:
    /// Create an abstract request editor retaining its source and callback target.
    HttpClientRequestEventEditor(event::EventSourcePtr source, event::EventsPtr target) :
        CommonEventEditor{std::move(source), std::move(target)} {}
};

}
