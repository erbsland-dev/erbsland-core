// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HttpClientResponse_fwd.hpp"

#include "../../../http_client/HttpClientResponseEventEditor.hpp"

namespace erbsland::network::impl {

/// Built-in HTTP client response editor.
/// @tested{HttpClientTest}
class HttpClientResponseEventEditor final : public network::HttpClientResponseEventEditor {
public:
    /// Bind the editor to its owning response.
    HttpClientResponseEventEditor(event::EventSourcePtr source, event::EventsPtr target, HttpClientResponse &response);

public: // implement network::HttpClientResponseEventEditor
    auto onBodyData(NetworkDataFn callback) -> HttpClientResponseEventEditor & override;
    auto onBody(NetworkDataFn callback) -> HttpClientResponseEventEditor & override;
    auto onText(std::function<void(text::String)> callback) -> HttpClientResponseEventEditor & override;
    auto onJson(std::function<void(text::json::JsonValue)> callback) -> HttpClientResponseEventEditor & override;
    auto onTrailers(std::function<void(const HttpHeaders &)> callback) -> HttpClientResponseEventEditor & override;
    auto onBodyProgress(HttpClientBodyProgressFn callback) -> HttpClientResponseEventEditor & override;
    auto onBodyCompleted(NetworkEventFn callback) -> HttpClientResponseEventEditor & override;
    auto onFinal(NetworkEventFn callback) -> HttpClientResponseEventEditor & override;

private:
    HttpClientResponse &_response; ///< Owning response.
};

}
