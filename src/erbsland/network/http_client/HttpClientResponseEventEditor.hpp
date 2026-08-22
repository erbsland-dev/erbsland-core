// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HttpClientRequestFn.hpp"

#include "../http/HttpHeaders.hpp"
#include "../source/NetworkDataFn.hpp"
#include "../source/NetworkEventFn.hpp"

#include "../../event/impl/CommonEventEditor.hpp"
#include "../../text/json/JsonValue_fwd.hpp"
#include "../../text/String.hpp"

#include <functional>
#include <utility>

namespace erbsland::network {

/// Callback editor for a low-level HTTP client response body.
/// @tested{HttpClientTest}
class HttpClientResponseEventEditor : public event::impl::CommonEventEditor {
public:
    // defaults
    ~HttpClientResponseEventEditor() override = default;

public:
    /// Replace the streamed-body block handler.
    virtual auto onBodyData(NetworkDataFn callback) -> HttpClientResponseEventEditor & = 0;
    /// Replace the manually aggregated byte-body handler.
    virtual auto onBody(NetworkDataFn callback) -> HttpClientResponseEventEditor & = 0;
    /// Replace the manually aggregated strict UTF-8 body handler.
    virtual auto onText(std::function<void(text::String)> callback) -> HttpClientResponseEventEditor & = 0;
    /// Replace the manually aggregated JSON body handler.
    virtual auto onJson(std::function<void(text::json::JsonValue)> callback) -> HttpClientResponseEventEditor & = 0;
    /// Replace the trailer handler.
    virtual auto onTrailers(std::function<void(const HttpHeaders &)> callback) -> HttpClientResponseEventEditor & = 0;
    /// Replace the output-sink progress handler.
    virtual auto onBodyProgress(HttpClientBodyProgressFn callback) -> HttpClientResponseEventEditor & = 0;
    /// Replace the response-body completion handler.
    virtual auto onBodyCompleted(NetworkEventFn callback) -> HttpClientResponseEventEditor & = 0;
    /// Replace the exactly-once response final handler.
    virtual auto onFinal(NetworkEventFn callback) -> HttpClientResponseEventEditor & = 0;

protected:
    /// Create an abstract response editor retaining its source and callback target.
    HttpClientResponseEventEditor(event::EventSourcePtr source, event::EventsPtr target) :
        CommonEventEditor{std::move(source), std::move(target)} {}
};

}
