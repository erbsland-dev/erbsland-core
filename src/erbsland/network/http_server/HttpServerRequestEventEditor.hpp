// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HttpServerRequestFn.hpp"

#include "../http/HttpHeaders.hpp"
#include "../source/NetworkDataFn.hpp"
#include "../source/NetworkErrorFn.hpp"
#include "../source/NetworkEventFn.hpp"

#include "../../event/impl/CommonEventEditor.hpp"

#include <functional>
#include <utility>

namespace erbsland::network {

/// Callback editor for one HTTP server request.
/// @tested{HttpServerLiveTest}
class HttpServerRequestEventEditor : public event::impl::CommonEventEditor {
public:
    // defaults
    ~HttpServerRequestEventEditor() override = default;

public:
    /// Replace the callback emitted after the final response head is committed.
    virtual auto onResponseCommitted(HttpServerResponseFn callback) -> HttpServerRequestEventEditor & = 0;
    /// Replace the callback emitted for an error affecting this request.
    virtual auto onError(NetworkErrorFn callback) -> HttpServerRequestEventEditor & = 0;
    /// Replace the streamed-body block callback.
    virtual auto onBodyData(NetworkDataFn callback) -> HttpServerRequestEventEditor & = 0;
    /// Replace the bounded aggregated-body callback.
    virtual auto onBody(NetworkDataFn callback) -> HttpServerRequestEventEditor & = 0;
    /// Replace the decoded-trailer callback.
    virtual auto onTrailers(std::function<void(const HttpHeaders &)> callback) -> HttpServerRequestEventEditor & = 0;
    /// Replace the incoming-body completion callback.
    virtual auto onBodyCompleted(NetworkEventFn callback) -> HttpServerRequestEventEditor & = 0;
    /// Replace the semantic response-writable callback.
    virtual auto onWritable(NetworkEventFn callback) -> HttpServerRequestEventEditor & = 0;
    /// Replace the exactly-once request final callback.
    virtual auto onFinal(NetworkEventFn callback) -> HttpServerRequestEventEditor & = 0;

protected:
    /// Create an abstract editor retaining its source and callback target.
    HttpServerRequestEventEditor(event::EventSourcePtr source, event::EventsPtr target) :
        CommonEventEditor{std::move(source), std::move(target)} {}
};

}
