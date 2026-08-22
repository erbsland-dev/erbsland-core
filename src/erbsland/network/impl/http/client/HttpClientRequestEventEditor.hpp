// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HttpClientRequest_fwd.hpp"

#include "../../../http_client/HttpClientRequestEventEditor.hpp"

namespace erbsland::network::impl {

/// Built-in HTTP client request editor.
/// @tested{HttpClientTest}
class HttpClientRequestEventEditor final : public network::HttpClientRequestEventEditor {
public:
    /// Bind the editor to its owning request.
    HttpClientRequestEventEditor(event::EventSourcePtr source, event::EventsPtr target, HttpClientRequest &request);

public: // implement network::HttpClientRequestEventEditor
    auto onResponse(HttpClientResponseFn callback, HttpClientResponseOptions options)
        -> HttpClientRequestEventEditor & override;
    auto onTextResponse(HttpClientTextResponseFn callback, HttpClientResponseOptions options)
        -> HttpClientRequestEventEditor & override;
    auto onJsonResponse(HttpClientJsonResponseFn callback, HttpClientResponseOptions options)
        -> HttpClientRequestEventEditor & override;
    auto onResponseHead(HttpClientResponseHeadFn callback) -> HttpClientRequestEventEditor & override;
    auto onInformationalResponse(HttpClientInformationalResponseFn callback) -> HttpClientRequestEventEditor & override;
    auto onRedirect(HttpClientRedirectFn callback) -> HttpClientRequestEventEditor & override;
    auto onWritable(NetworkEventFn callback) -> HttpClientRequestEventEditor & override;
    auto onError(HttpClientErrorFn callback) -> HttpClientRequestEventEditor & override;
    auto onFinal(NetworkEventFn callback) -> HttpClientRequestEventEditor & override;

private:
    HttpClientRequest &_request; ///< Owning request.
};

}
