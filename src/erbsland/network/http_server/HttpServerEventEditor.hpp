// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HttpServerRequestFn.hpp"
#include "HttpServerRouteOptions.hpp"

#include "../http/HttpMethod.hpp"
#include "../http/HttpMethodType.hpp"
#include "../source/NetworkErrorFn.hpp"
#include "../source/NetworkEventFn.hpp"

#include "../../event/impl/CommonEventEditor.hpp"
#include "../../text/String.hpp"

#include <utility>

namespace erbsland::network {

/// Callback and route editor for an HTTP server.
/// @tested{HttpServerLiveTest}
class HttpServerEventEditor : public event::impl::CommonEventEditor {
public:
    // defaults
    ~HttpServerEventEditor() override = default;

public:
    /// Replace the callback emitted after binding completes.
    virtual auto onListening(NetworkEventFn callback) -> HttpServerEventEditor & = 0;
    /// Replace the callback emitted before a newly created session is routed.
    virtual auto onNewSession(HttpServerSessionFn callback) -> HttpServerEventEditor & = 0;
    /// Add an aggregated byte route for every valid method.
    virtual auto onRequest(text::String pattern, HttpServerRequestFn callback, HttpServerRouteOptions options = {})
        -> HttpServerEventEditor & = 0;
    /// Add an aggregated byte route for one method.
    virtual auto onRequest(
        HttpMethod method, text::String pattern, HttpServerRequestFn callback, HttpServerRouteOptions options = {})
        -> HttpServerEventEditor & = 0;
    /// Add an aggregated byte route for standard method flags.
    virtual auto onRequest(
        HttpMethodTypes methods,
        text::String pattern,
        HttpServerRequestFn callback,
        HttpServerRouteOptions options = {}) -> HttpServerEventEditor & = 0;
    /// Add an aggregated strict UTF-8 route for every valid method.
    virtual auto onTextRequest(
        text::String pattern, HttpServerTextRequestFn callback, HttpServerRouteOptions options = {})
        -> HttpServerEventEditor & = 0;
    /// Add an aggregated strict UTF-8 route for one method.
    virtual auto onTextRequest(
        HttpMethod method, text::String pattern, HttpServerTextRequestFn callback, HttpServerRouteOptions options = {})
        -> HttpServerEventEditor & = 0;
    /// Add an aggregated strict UTF-8 route for standard method flags.
    virtual auto onTextRequest(
        HttpMethodTypes methods,
        text::String pattern,
        HttpServerTextRequestFn callback,
        HttpServerRouteOptions options = {}) -> HttpServerEventEditor & = 0;
    /// Add an aggregated JSON route for every valid method.
    virtual auto onJsonRequest(
        text::String pattern, HttpServerJsonRequestFn callback, HttpServerRouteOptions options = {})
        -> HttpServerEventEditor & = 0;
    /// Add an aggregated JSON route for one method.
    virtual auto onJsonRequest(
        HttpMethod method, text::String pattern, HttpServerJsonRequestFn callback, HttpServerRouteOptions options = {})
        -> HttpServerEventEditor & = 0;
    /// Add an aggregated JSON route for standard method flags.
    virtual auto onJsonRequest(
        HttpMethodTypes methods,
        text::String pattern,
        HttpServerJsonRequestFn callback,
        HttpServerRouteOptions options = {}) -> HttpServerEventEditor & = 0;
    /// Add a low-level request-head route for every valid method.
    virtual auto onRequestHead(text::String pattern, HttpServerRequestHeadFn callback) -> HttpServerEventEditor & = 0;
    /// Add a low-level request-head route for one method.
    virtual auto onRequestHead(HttpMethod method, text::String pattern, HttpServerRequestHeadFn callback)
        -> HttpServerEventEditor & = 0;
    /// Add a low-level request-head route for standard method flags.
    virtual auto onRequestHead(HttpMethodTypes methods, text::String pattern, HttpServerRequestHeadFn callback)
        -> HttpServerEventEditor & = 0;
    /// Add the only patternless fallback route family.
    virtual auto onRequestHead(HttpServerRequestHeadFn callback) -> HttpServerEventEditor & = 0;
    /// Replace the graceful-closure callback.
    virtual auto onClosed(NetworkEventFn callback) -> HttpServerEventEditor & = 0;
    /// Replace the operational-error callback.
    virtual auto onError(NetworkErrorFn callback) -> HttpServerEventEditor & = 0;
    /// Replace the exactly-once final callback.
    virtual auto onFinal(NetworkEventFn callback) -> HttpServerEventEditor & = 0;

protected:
    /// Create an abstract editor retaining its source and callback target.
    HttpServerEventEditor(event::EventSourcePtr source, event::EventsPtr target) :
        CommonEventEditor{std::move(source), std::move(target)} {}
};

}
