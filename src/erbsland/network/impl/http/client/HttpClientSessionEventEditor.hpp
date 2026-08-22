// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HttpClientSession_fwd.hpp"

#include "../../../http_client/HttpClientSessionEventEditor.hpp"

namespace erbsland::network::impl {

/// Built-in HTTP client session editor.
/// @tested{HttpClientTest}
class HttpClientSessionEventEditor final : public network::HttpClientSessionEventEditor {
public:
    /// Bind the editor to its owning session.
    HttpClientSessionEventEditor(event::EventSourcePtr source, event::EventsPtr target, HttpClientSession &session);

public: // implement network::HttpClientSessionEventEditor
    auto onResponse(HttpClientResponseFn callback, HttpClientResponseOptions options)
        -> HttpClientSessionEventEditor & override;
    auto onTextResponse(HttpClientTextResponseFn callback, HttpClientResponseOptions options)
        -> HttpClientSessionEventEditor & override;
    auto onJsonResponse(HttpClientJsonResponseFn callback, HttpClientResponseOptions options)
        -> HttpClientSessionEventEditor & override;
    auto onResponseHead(HttpClientResponseHeadFn callback) -> HttpClientSessionEventEditor & override;
    auto onInformationalResponse(HttpClientInformationalResponseFn callback) -> HttpClientSessionEventEditor & override;
    auto onRedirect(HttpClientRedirectFn callback) -> HttpClientSessionEventEditor & override;
    auto onError(HttpClientErrorFn callback) -> HttpClientSessionEventEditor & override;
    auto onClosed(NetworkEventFn callback) -> HttpClientSessionEventEditor & override;
    auto onFinal(NetworkEventFn callback) -> HttpClientSessionEventEditor & override;

private:
    HttpClientSession &_session; ///< Owning session.
};

}
