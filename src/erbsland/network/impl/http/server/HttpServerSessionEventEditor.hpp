// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HttpServerSession_fwd.hpp"
#include "HttpServerSessionEventEditor_fwd.hpp"

#include "../../../http_server/HttpServerSessionEventEditor.hpp"

namespace erbsland::network::impl {

/// Built-in HTTP session editor.
/// @tested{HttpServerSessionTest HttpServerLiveTest}
class HttpServerSessionEventEditor final : public network::HttpServerSessionEventEditor {
public:
    /// Bind the stable editor to its owning session.
    HttpServerSessionEventEditor(event::EventSourcePtr source, event::EventsPtr target, HttpServerSession &session);

public: // implement network::HttpServerSessionEventEditor
    auto onRequest(text::String pattern, HttpServerRequestFn callback, HttpServerRouteOptions options)
        -> HttpServerSessionEventEditor & override;
    auto onRequest(
        HttpMethod method, text::String pattern, HttpServerRequestFn callback, HttpServerRouteOptions options)
        -> HttpServerSessionEventEditor & override;
    auto onRequest(
        HttpMethodTypes methods, text::String pattern, HttpServerRequestFn callback, HttpServerRouteOptions options)
        -> HttpServerSessionEventEditor & override;
    auto onTextRequest(text::String pattern, HttpServerTextRequestFn callback, HttpServerRouteOptions options)
        -> HttpServerSessionEventEditor & override;
    auto onTextRequest(
        HttpMethod method, text::String pattern, HttpServerTextRequestFn callback, HttpServerRouteOptions options)
        -> HttpServerSessionEventEditor & override;
    auto onTextRequest(
        HttpMethodTypes methods, text::String pattern, HttpServerTextRequestFn callback, HttpServerRouteOptions options)
        -> HttpServerSessionEventEditor & override;
    auto onJsonRequest(text::String pattern, HttpServerJsonRequestFn callback, HttpServerRouteOptions options)
        -> HttpServerSessionEventEditor & override;
    auto onJsonRequest(
        HttpMethod method, text::String pattern, HttpServerJsonRequestFn callback, HttpServerRouteOptions options)
        -> HttpServerSessionEventEditor & override;
    auto onJsonRequest(
        HttpMethodTypes methods, text::String pattern, HttpServerJsonRequestFn callback, HttpServerRouteOptions options)
        -> HttpServerSessionEventEditor & override;
    auto onRequestHead(text::String pattern, HttpServerRequestHeadFn callback)
        -> HttpServerSessionEventEditor & override;
    auto onRequestHead(HttpMethod method, text::String pattern, HttpServerRequestHeadFn callback)
        -> HttpServerSessionEventEditor & override;
    auto onRequestHead(HttpMethodTypes methods, text::String pattern, HttpServerRequestHeadFn callback)
        -> HttpServerSessionEventEditor & override;
    auto onRequestHead(HttpServerRequestHeadFn callback) -> HttpServerSessionEventEditor & override;
    auto onInvalidated(NetworkEventFn callback) -> HttpServerSessionEventEditor & override;
    auto onFinal(NetworkEventFn callback) -> HttpServerSessionEventEditor & override;

private:
    HttpServerSession &_session; ///< Owning session.
};

}
