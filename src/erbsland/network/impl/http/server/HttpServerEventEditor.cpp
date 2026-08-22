// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "HttpServerEventEditor.hpp"

#include "HttpServer.hpp"

namespace erbsland::network::impl {

HttpServerEventEditor::HttpServerEventEditor(
    event::EventSourcePtr source, event::EventsPtr target, HttpServer &server) :
    network::HttpServerEventEditor{std::move(source), std::move(target)}, _server{server} {
}

auto HttpServerEventEditor::onListening(NetworkEventFn callback) -> HttpServerEventEditor & {
    _server._onListening = std::move(callback);
    return *this;
}
auto HttpServerEventEditor::onNewSession(HttpServerSessionFn callback) -> HttpServerEventEditor & {
    _server._onNewSession = std::move(callback);
    return *this;
}
auto HttpServerEventEditor::onRequest(
    text::String pattern, HttpServerRequestFn callback, HttpServerRouteOptions options) -> HttpServerEventEditor & {
    _server._routes.add(std::move(pattern), HttpRouteHandler{std::move(callback), std::move(options)});
    return *this;
}
auto HttpServerEventEditor::onRequest(
    HttpMethod method, text::String pattern, HttpServerRequestFn callback, HttpServerRouteOptions options)
    -> HttpServerEventEditor & {
    _server._routes.add(
        std::move(method), std::move(pattern), HttpRouteHandler{std::move(callback), std::move(options)});
    return *this;
}
auto HttpServerEventEditor::onRequest(
    const HttpMethodTypes methods, text::String pattern, HttpServerRequestFn callback, HttpServerRouteOptions options)
    -> HttpServerEventEditor & {
    _server._routes.add(methods, std::move(pattern), HttpRouteHandler{std::move(callback), std::move(options)});
    return *this;
}
auto HttpServerEventEditor::onTextRequest(
    text::String pattern, HttpServerTextRequestFn callback, HttpServerRouteOptions options) -> HttpServerEventEditor & {
    _server._routes.add(std::move(pattern), HttpRouteHandler{std::move(callback), std::move(options)});
    return *this;
}
auto HttpServerEventEditor::onTextRequest(
    HttpMethod method, text::String pattern, HttpServerTextRequestFn callback, HttpServerRouteOptions options)
    -> HttpServerEventEditor & {
    _server._routes.add(
        std::move(method), std::move(pattern), HttpRouteHandler{std::move(callback), std::move(options)});
    return *this;
}
auto HttpServerEventEditor::onTextRequest(
    const HttpMethodTypes methods,
    text::String pattern,
    HttpServerTextRequestFn callback,
    HttpServerRouteOptions options) -> HttpServerEventEditor & {
    _server._routes.add(methods, std::move(pattern), HttpRouteHandler{std::move(callback), std::move(options)});
    return *this;
}
auto HttpServerEventEditor::onJsonRequest(
    text::String pattern, HttpServerJsonRequestFn callback, HttpServerRouteOptions options) -> HttpServerEventEditor & {
    _server._routes.add(std::move(pattern), HttpRouteHandler{std::move(callback), std::move(options)});
    return *this;
}
auto HttpServerEventEditor::onJsonRequest(
    HttpMethod method, text::String pattern, HttpServerJsonRequestFn callback, HttpServerRouteOptions options)
    -> HttpServerEventEditor & {
    _server._routes.add(
        std::move(method), std::move(pattern), HttpRouteHandler{std::move(callback), std::move(options)});
    return *this;
}
auto HttpServerEventEditor::onJsonRequest(
    const HttpMethodTypes methods,
    text::String pattern,
    HttpServerJsonRequestFn callback,
    HttpServerRouteOptions options) -> HttpServerEventEditor & {
    _server._routes.add(methods, std::move(pattern), HttpRouteHandler{std::move(callback), std::move(options)});
    return *this;
}
auto HttpServerEventEditor::onRequestHead(text::String pattern, HttpServerRequestHeadFn callback)
    -> HttpServerEventEditor & {
    _server._routes.add(std::move(pattern), HttpRouteHandler{std::move(callback)});
    return *this;
}
auto HttpServerEventEditor::onRequestHead(HttpMethod method, text::String pattern, HttpServerRequestHeadFn callback)
    -> HttpServerEventEditor & {
    _server._routes.add(std::move(method), std::move(pattern), HttpRouteHandler{std::move(callback)});
    return *this;
}
auto HttpServerEventEditor::onRequestHead(
    const HttpMethodTypes methods, text::String pattern, HttpServerRequestHeadFn callback) -> HttpServerEventEditor & {
    _server._routes.add(methods, std::move(pattern), HttpRouteHandler{std::move(callback)});
    return *this;
}
auto HttpServerEventEditor::onRequestHead(HttpServerRequestHeadFn callback) -> HttpServerEventEditor & {
    _server._routes.addFallback(HttpRouteHandler{std::move(callback)});
    return *this;
}
auto HttpServerEventEditor::onClosed(NetworkEventFn callback) -> HttpServerEventEditor & {
    _server._onClosed = std::move(callback);
    return *this;
}
auto HttpServerEventEditor::onError(NetworkErrorFn callback) -> HttpServerEventEditor & {
    _server._onError = std::move(callback);
    return *this;
}
auto HttpServerEventEditor::onFinal(NetworkEventFn callback) -> HttpServerEventEditor & {
    _server._onFinal = std::move(callback);
    return *this;
}

}
