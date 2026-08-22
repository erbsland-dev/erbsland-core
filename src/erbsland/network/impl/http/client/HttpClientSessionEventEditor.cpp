// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "HttpClientSessionEventEditor.hpp"

#include "HttpClientResponseHandler.hpp"
#include "HttpClientSession.hpp"

namespace erbsland::network::impl {

HttpClientSessionEventEditor::HttpClientSessionEventEditor(
    event::EventSourcePtr source, event::EventsPtr target, HttpClientSession &session) :
    network::HttpClientSessionEventEditor{std::move(source), std::move(target)}, _session{session} {
}

auto HttpClientSessionEventEditor::onResponse(HttpClientResponseFn callback, HttpClientResponseOptions options)
    -> HttpClientSessionEventEditor & {
    _session._responseHandler = HttpClientResponseHandler{std::move(callback), std::move(options)};
    return *this;
}

auto HttpClientSessionEventEditor::onTextResponse(HttpClientTextResponseFn callback, HttpClientResponseOptions options)
    -> HttpClientSessionEventEditor & {
    _session._responseHandler = HttpClientResponseHandler{std::move(callback), std::move(options)};
    return *this;
}

auto HttpClientSessionEventEditor::onJsonResponse(HttpClientJsonResponseFn callback, HttpClientResponseOptions options)
    -> HttpClientSessionEventEditor & {
    _session._responseHandler = HttpClientResponseHandler{std::move(callback), std::move(options)};
    return *this;
}

auto HttpClientSessionEventEditor::onResponseHead(HttpClientResponseHeadFn callback) -> HttpClientSessionEventEditor & {
    _session._responseHandler = HttpClientResponseHandler{std::move(callback)};
    return *this;
}

auto HttpClientSessionEventEditor::onInformationalResponse(HttpClientInformationalResponseFn callback)
    -> HttpClientSessionEventEditor & {
    _session._onInformational = std::move(callback);
    return *this;
}

auto HttpClientSessionEventEditor::onRedirect(HttpClientRedirectFn callback) -> HttpClientSessionEventEditor & {
    _session._onRedirect = std::move(callback);
    return *this;
}

auto HttpClientSessionEventEditor::onError(HttpClientErrorFn callback) -> HttpClientSessionEventEditor & {
    _session._onError = std::move(callback);
    return *this;
}

auto HttpClientSessionEventEditor::onClosed(NetworkEventFn callback) -> HttpClientSessionEventEditor & {
    _session._onClosed = std::move(callback);
    return *this;
}

auto HttpClientSessionEventEditor::onFinal(NetworkEventFn callback) -> HttpClientSessionEventEditor & {
    _session._onFinal = std::move(callback);
    return *this;
}

}
