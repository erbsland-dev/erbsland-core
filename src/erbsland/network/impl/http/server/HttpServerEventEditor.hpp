// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HttpServer_fwd.hpp"
#include "HttpServerEventEditor_fwd.hpp"

#include "../../../http_server/HttpServerEventEditor.hpp"

namespace erbsland::network::impl {

/// Built-in HTTP server event and route editor.
/// @tested{HttpServerLiveTest}
class HttpServerEventEditor final : public network::HttpServerEventEditor {
public:
    /// Bind the stable editor to its owning server.
    HttpServerEventEditor(event::EventSourcePtr source, event::EventsPtr target, HttpServer &server);

public: // implement network::HttpServerEventEditor
    auto onListening(NetworkEventFn callback) -> HttpServerEventEditor & override;
    auto onNewSession(HttpServerSessionFn callback) -> HttpServerEventEditor & override;
    auto onRequest(text::String pattern, HttpServerRequestFn callback, HttpServerRouteOptions options)
        -> HttpServerEventEditor & override;
    auto onRequest(
        HttpMethod method, text::String pattern, HttpServerRequestFn callback, HttpServerRouteOptions options)
        -> HttpServerEventEditor & override;
    auto onRequest(
        HttpMethodTypes methods, text::String pattern, HttpServerRequestFn callback, HttpServerRouteOptions options)
        -> HttpServerEventEditor & override;
    auto onTextRequest(text::String pattern, HttpServerTextRequestFn callback, HttpServerRouteOptions options)
        -> HttpServerEventEditor & override;
    auto onTextRequest(
        HttpMethod method, text::String pattern, HttpServerTextRequestFn callback, HttpServerRouteOptions options)
        -> HttpServerEventEditor & override;
    auto onTextRequest(
        HttpMethodTypes methods, text::String pattern, HttpServerTextRequestFn callback, HttpServerRouteOptions options)
        -> HttpServerEventEditor & override;
    auto onJsonRequest(text::String pattern, HttpServerJsonRequestFn callback, HttpServerRouteOptions options)
        -> HttpServerEventEditor & override;
    auto onJsonRequest(
        HttpMethod method, text::String pattern, HttpServerJsonRequestFn callback, HttpServerRouteOptions options)
        -> HttpServerEventEditor & override;
    auto onJsonRequest(
        HttpMethodTypes methods, text::String pattern, HttpServerJsonRequestFn callback, HttpServerRouteOptions options)
        -> HttpServerEventEditor & override;
    auto onRequestHead(text::String pattern, HttpServerRequestHeadFn callback) -> HttpServerEventEditor & override;
    auto onRequestHead(HttpMethod method, text::String pattern, HttpServerRequestHeadFn callback)
        -> HttpServerEventEditor & override;
    auto onRequestHead(HttpMethodTypes methods, text::String pattern, HttpServerRequestHeadFn callback)
        -> HttpServerEventEditor & override;
    auto onRequestHead(HttpServerRequestHeadFn callback) -> HttpServerEventEditor & override;
    auto onClosed(NetworkEventFn callback) -> HttpServerEventEditor & override;
    auto onError(NetworkErrorFn callback) -> HttpServerEventEditor & override;
    auto onFinal(NetworkEventFn callback) -> HttpServerEventEditor & override;

private:
    HttpServer &_server; ///< Owning server.
};

}
