// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "HttpServerRequestEventEditor.hpp"

#include "HttpServerRequest.hpp"

namespace erbsland::network::impl {

HttpServerRequestEventEditor::HttpServerRequestEventEditor(
    event::EventSourcePtr source, event::EventsPtr target, HttpServerRequest &request) :
    network::HttpServerRequestEventEditor{std::move(source), std::move(target)}, _request{request} {
}

auto HttpServerRequestEventEditor::onBodyData(NetworkDataFn callback) -> HttpServerRequestEventEditor & {
    _request._onBodyData = std::move(callback);
    return *this;
}

auto HttpServerRequestEventEditor::onBody(NetworkDataFn callback) -> HttpServerRequestEventEditor & {
    _request._onBody = std::move(callback);
    return *this;
}

auto HttpServerRequestEventEditor::onTrailers(std::function<void(const HttpHeaders &)> callback)
    -> HttpServerRequestEventEditor & {
    _request._onTrailers = std::move(callback);
    return *this;
}

auto HttpServerRequestEventEditor::onBodyCompleted(NetworkEventFn callback) -> HttpServerRequestEventEditor & {
    _request._onBodyCompleted = std::move(callback);
    return *this;
}

auto HttpServerRequestEventEditor::onWritable(NetworkEventFn callback) -> HttpServerRequestEventEditor & {
    _request._onWritable = std::move(callback);
    return *this;
}

auto HttpServerRequestEventEditor::onFinal(NetworkEventFn callback) -> HttpServerRequestEventEditor & {
    _request._onFinal = std::move(callback);
    return *this;
}

}
