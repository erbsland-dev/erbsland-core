// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "HttpClientRequestEventEditor.hpp"

#include "HttpClientRequest.hpp"
#include "HttpClientResponseHandler.hpp"

namespace erbsland::network::impl {

HttpClientRequestEventEditor::HttpClientRequestEventEditor(
    event::EventSourcePtr source, event::EventsPtr target, HttpClientRequest &request) :
    network::HttpClientRequestEventEditor{std::move(source), std::move(target)}, _request{request} {
}

auto HttpClientRequestEventEditor::onResponse(HttpClientResponseFn callback, HttpClientResponseOptions options)
    -> HttpClientRequestEventEditor & {
    _request._requestResponseHandler = HttpClientResponseHandler{std::move(callback), std::move(options)};
    return *this;
}

auto HttpClientRequestEventEditor::onTextResponse(HttpClientTextResponseFn callback, HttpClientResponseOptions options)
    -> HttpClientRequestEventEditor & {
    _request._requestResponseHandler = HttpClientResponseHandler{std::move(callback), std::move(options)};
    return *this;
}

auto HttpClientRequestEventEditor::onJsonResponse(HttpClientJsonResponseFn callback, HttpClientResponseOptions options)
    -> HttpClientRequestEventEditor & {
    _request._requestResponseHandler = HttpClientResponseHandler{std::move(callback), std::move(options)};
    return *this;
}

auto HttpClientRequestEventEditor::onResponseHead(HttpClientResponseHeadFn callback) -> HttpClientRequestEventEditor & {
    _request._requestResponseHandler = HttpClientResponseHandler{std::move(callback)};
    return *this;
}

auto HttpClientRequestEventEditor::onInformationalResponse(HttpClientInformationalResponseFn callback)
    -> HttpClientRequestEventEditor & {
    _request._requestInformational = std::move(callback);
    return *this;
}

auto HttpClientRequestEventEditor::onRedirect(HttpClientRedirectFn callback) -> HttpClientRequestEventEditor & {
    _request._requestRedirect = std::move(callback);
    return *this;
}

auto HttpClientRequestEventEditor::onWritable(NetworkEventFn callback) -> HttpClientRequestEventEditor & {
    _request._onWritable = std::move(callback);
    return *this;
}

auto HttpClientRequestEventEditor::onError(HttpClientErrorFn callback) -> HttpClientRequestEventEditor & {
    _request._requestError = std::move(callback);
    return *this;
}

auto HttpClientRequestEventEditor::onFinal(NetworkEventFn callback) -> HttpClientRequestEventEditor & {
    _request._onFinal = std::move(callback);
    return *this;
}

}
