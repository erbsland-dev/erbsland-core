// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "HttpClientResponseEventEditor.hpp"

#include "HttpClientResponse.hpp"

namespace erbsland::network::impl {

HttpClientResponseEventEditor::HttpClientResponseEventEditor(
    event::EventSourcePtr source, event::EventsPtr target, HttpClientResponse &response) :
    network::HttpClientResponseEventEditor{std::move(source), std::move(target)}, _response{response} {
}

auto HttpClientResponseEventEditor::onBodyData(NetworkDataFn callback) -> HttpClientResponseEventEditor & {
    _response._onBodyData = std::move(callback);
    return *this;
}

auto HttpClientResponseEventEditor::onBody(NetworkDataFn callback) -> HttpClientResponseEventEditor & {
    _response._onBody = std::move(callback);
    return *this;
}

auto HttpClientResponseEventEditor::onText(std::function<void(text::String)> callback)
    -> HttpClientResponseEventEditor & {
    _response._onText = std::move(callback);
    return *this;
}

auto HttpClientResponseEventEditor::onJson(std::function<void(text::json::JsonValue)> callback)
    -> HttpClientResponseEventEditor & {
    _response._onJson = std::move(callback);
    return *this;
}

auto HttpClientResponseEventEditor::onTrailers(std::function<void(const HttpHeaders &)> callback)
    -> HttpClientResponseEventEditor & {
    _response._onTrailers = std::move(callback);
    return *this;
}

auto HttpClientResponseEventEditor::onBodyProgress(HttpClientBodyProgressFn callback)
    -> HttpClientResponseEventEditor & {
    _response._onBodyProgress = std::move(callback);
    return *this;
}

auto HttpClientResponseEventEditor::onBodyCompleted(NetworkEventFn callback) -> HttpClientResponseEventEditor & {
    _response._onBodyCompleted = std::move(callback);
    return *this;
}

auto HttpClientResponseEventEditor::onFinal(NetworkEventFn callback) -> HttpClientResponseEventEditor & {
    _response._onFinal = std::move(callback);
    return *this;
}

}
