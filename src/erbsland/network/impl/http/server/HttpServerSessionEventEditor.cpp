// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "HttpServerSessionEventEditor.hpp"

#include "HttpServerSession.hpp"

namespace erbsland::network::impl {

using namespace text;

HttpServerSessionEventEditor::HttpServerSessionEventEditor(
    event::EventSourcePtr source, event::EventTargetPtr target, HttpServerSession &session) :
    network::HttpServerSessionEventEditor{std::move(source), std::move(target)}, _session{session} {
}

auto HttpServerSessionEventEditor::onRequestReceived(HttpServerRequestEventFn callback)
    -> HttpServerSessionEventEditor & {
    _session._onRequestReceived = std::move(callback);
    return *this;
}

auto HttpServerSessionEventEditor::onRequest(
    String pattern, HttpServerRequestFn callback, HttpServerRouteOptions options) -> HttpServerSessionEventEditor & {
    _session.routes().add(std::move(pattern), HttpRouteHandler{std::move(callback), std::move(options)});
    return *this;
}

auto HttpServerSessionEventEditor::onRequest(
    HttpMethod method, String pattern, HttpServerRequestFn callback, HttpServerRouteOptions options)
    -> HttpServerSessionEventEditor & {
    _session.routes().add(
        std::move(method), std::move(pattern), HttpRouteHandler{std::move(callback), std::move(options)});
    return *this;
}

auto HttpServerSessionEventEditor::onRequest(
    const HttpMethodTypes methods, String pattern, HttpServerRequestFn callback, HttpServerRouteOptions options)
    -> HttpServerSessionEventEditor & {
    _session.routes().add(methods, std::move(pattern), HttpRouteHandler{std::move(callback), std::move(options)});
    return *this;
}

auto HttpServerSessionEventEditor::onTextRequest(
    String pattern, HttpServerTextRequestFn callback, HttpServerRouteOptions options)
    -> HttpServerSessionEventEditor & {
    _session.routes().add(std::move(pattern), HttpRouteHandler{std::move(callback), std::move(options)});
    return *this;
}

auto HttpServerSessionEventEditor::onTextRequest(
    HttpMethod method, String pattern, HttpServerTextRequestFn callback, HttpServerRouteOptions options)
    -> HttpServerSessionEventEditor & {
    _session.routes().add(
        std::move(method), std::move(pattern), HttpRouteHandler{std::move(callback), std::move(options)});
    return *this;
}

auto HttpServerSessionEventEditor::onTextRequest(
    const HttpMethodTypes methods, String pattern, HttpServerTextRequestFn callback, HttpServerRouteOptions options)
    -> HttpServerSessionEventEditor & {
    _session.routes().add(methods, std::move(pattern), HttpRouteHandler{std::move(callback), std::move(options)});
    return *this;
}

auto HttpServerSessionEventEditor::onJsonRequest(
    String pattern, HttpServerJsonRequestFn callback, HttpServerRouteOptions options)
    -> HttpServerSessionEventEditor & {
    _session.routes().add(std::move(pattern), HttpRouteHandler{std::move(callback), std::move(options)});
    return *this;
}

auto HttpServerSessionEventEditor::onJsonRequest(
    HttpMethod method, String pattern, HttpServerJsonRequestFn callback, HttpServerRouteOptions options)
    -> HttpServerSessionEventEditor & {
    _session.routes().add(
        std::move(method), std::move(pattern), HttpRouteHandler{std::move(callback), std::move(options)});
    return *this;
}

auto HttpServerSessionEventEditor::onJsonRequest(
    const HttpMethodTypes methods, String pattern, HttpServerJsonRequestFn callback, HttpServerRouteOptions options)
    -> HttpServerSessionEventEditor & {
    _session.routes().add(methods, std::move(pattern), HttpRouteHandler{std::move(callback), std::move(options)});
    return *this;
}

auto HttpServerSessionEventEditor::onRequestHead(String pattern, HttpServerRequestHeadFn callback)
    -> HttpServerSessionEventEditor & {
    _session.routes().add(std::move(pattern), HttpRouteHandler{std::move(callback)});
    return *this;
}

auto HttpServerSessionEventEditor::onRequestHead(HttpMethod method, String pattern, HttpServerRequestHeadFn callback)
    -> HttpServerSessionEventEditor & {
    _session.routes().add(std::move(method), std::move(pattern), HttpRouteHandler{std::move(callback)});
    return *this;
}

auto HttpServerSessionEventEditor::onRequestHead(
    const HttpMethodTypes methods, String pattern, HttpServerRequestHeadFn callback) -> HttpServerSessionEventEditor & {
    _session.routes().add(methods, std::move(pattern), HttpRouteHandler{std::move(callback)});
    return *this;
}

auto HttpServerSessionEventEditor::onRequestHead(HttpServerRequestHeadFn callback) -> HttpServerSessionEventEditor & {
    _session.routes().addFallback(HttpRouteHandler{std::move(callback)});
    return *this;
}

auto HttpServerSessionEventEditor::onInvalidated(NetworkEventFn callback) -> HttpServerSessionEventEditor & {
    _session._onInvalidated = std::move(callback);
    return *this;
}

auto HttpServerSessionEventEditor::onFinal(NetworkEventFn callback) -> HttpServerSessionEventEditor & {
    _session._onFinal = std::move(callback);
    return *this;
}

}
