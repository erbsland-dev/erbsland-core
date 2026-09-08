// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HttpServerRequestFn.hpp"
#include "HttpServerRouteOptions.hpp"

#include "../http/HttpMethod.hpp"
#include "../http/HttpMethodType.hpp"
#include "../source/NetworkEventFn.hpp"

#include "../../event/impl/CommonEventEditor.hpp"
#include "../../text/String.hpp"

#include <optional>
#include <utility>
#include <vector>

namespace erbsland::network {

/// Callback and route editor for one HTTP server session.
/// @tested{HttpServerSessionTest HttpServerLiveTest}
class HttpServerSessionEventEditor : public event::impl::CommonEventEditor {
public:
    // defaults
    ~HttpServerSessionEventEditor() override = default;

public:
    /// Replace the callback emitted when a request is selected for this session.
    virtual auto onRequestReceived(HttpServerRequestEventFn callback) -> HttpServerSessionEventEditor & = 0;
    /// Add an aggregated byte route for every method.
    virtual auto onRequest(text::String pattern, HttpServerRequestFn callback, HttpServerRouteOptions options = {})
        -> HttpServerSessionEventEditor & = 0;
    /// Add an aggregated byte route for one method.
    virtual auto onRequest(
        HttpMethod method, text::String pattern, HttpServerRequestFn callback, HttpServerRouteOptions options = {})
        -> HttpServerSessionEventEditor & = 0;
    /// Add an aggregated byte route for standard method flags.
    virtual auto onRequest(
        HttpMethodTypes methods,
        text::String pattern,
        HttpServerRequestFn callback,
        HttpServerRouteOptions options = {}) -> HttpServerSessionEventEditor & = 0;
    /// Add an aggregated strict UTF-8 text route for every method.
    virtual auto onTextRequest(
        text::String pattern, HttpServerTextRequestFn callback, HttpServerRouteOptions options = {})
        -> HttpServerSessionEventEditor & = 0;
    /// Add an aggregated strict UTF-8 text route for one method.
    virtual auto onTextRequest(
        HttpMethod method, text::String pattern, HttpServerTextRequestFn callback, HttpServerRouteOptions options = {})
        -> HttpServerSessionEventEditor & = 0;
    /// Add an aggregated strict UTF-8 text route for standard method flags.
    virtual auto onTextRequest(
        HttpMethodTypes methods,
        text::String pattern,
        HttpServerTextRequestFn callback,
        HttpServerRouteOptions options = {}) -> HttpServerSessionEventEditor & = 0;
    /// Add an aggregated JSON route for every method.
    virtual auto onJsonRequest(
        text::String pattern, HttpServerJsonRequestFn callback, HttpServerRouteOptions options = {})
        -> HttpServerSessionEventEditor & = 0;
    /// Add an aggregated JSON route for one method.
    virtual auto onJsonRequest(
        HttpMethod method, text::String pattern, HttpServerJsonRequestFn callback, HttpServerRouteOptions options = {})
        -> HttpServerSessionEventEditor & = 0;
    /// Add an aggregated JSON route for standard method flags.
    virtual auto onJsonRequest(
        HttpMethodTypes methods,
        text::String pattern,
        HttpServerJsonRequestFn callback,
        HttpServerRouteOptions options = {}) -> HttpServerSessionEventEditor & = 0;
    /// Add a low-level request-head route for every method.
    virtual auto onRequestHead(text::String pattern, HttpServerRequestHeadFn callback)
        -> HttpServerSessionEventEditor & = 0;
    /// Add a low-level request-head route for one method.
    virtual auto onRequestHead(HttpMethod method, text::String pattern, HttpServerRequestHeadFn callback)
        -> HttpServerSessionEventEditor & = 0;
    /// Add a low-level request-head route for standard method flags.
    virtual auto onRequestHead(HttpMethodTypes methods, text::String pattern, HttpServerRequestHeadFn callback)
        -> HttpServerSessionEventEditor & = 0;
    /// Add the low-level fallback invoked after all patterned routes fail.
    virtual auto onRequestHead(HttpServerRequestHeadFn callback) -> HttpServerSessionEventEditor & = 0;
    /// Replace the invalidation callback.
    virtual auto onInvalidated(NetworkEventFn callback) -> HttpServerSessionEventEditor & = 0;
    /// Replace the exactly-once session final callback.
    virtual auto onFinal(NetworkEventFn callback) -> HttpServerSessionEventEditor & = 0;

protected:
    /// Create an abstract editor retaining its source and callback target.
    HttpServerSessionEventEditor(event::EventSourcePtr source, event::EventsPtr target) :
        CommonEventEditor{std::move(source), std::move(target)} {}
};

}
