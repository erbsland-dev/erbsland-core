// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HttpServerRequest_fwd.hpp"
#include "HttpServerRequestEventEditor_fwd.hpp"

#include "../../../http_server/HttpServerRequestEventEditor.hpp"

namespace erbsland::network::impl {

/// Built-in request callback editor.
/// @tested{HttpServerLiveTest}
class HttpServerRequestEventEditor final : public network::HttpServerRequestEventEditor {
public:
    /// Bind the stable editor to its owning request.
    HttpServerRequestEventEditor(event::EventSourcePtr source, event::EventsPtr target, HttpServerRequest &request);

public: // implement network::HttpServerRequestEventEditor
    auto onBodyData(NetworkDataFn callback) -> HttpServerRequestEventEditor & override;
    auto onBody(NetworkDataFn callback) -> HttpServerRequestEventEditor & override;
    auto onTrailers(std::function<void(const HttpHeaders &)> callback) -> HttpServerRequestEventEditor & override;
    auto onBodyCompleted(NetworkEventFn callback) -> HttpServerRequestEventEditor & override;
    auto onWritable(NetworkEventFn callback) -> HttpServerRequestEventEditor & override;
    auto onFinal(NetworkEventFn callback) -> HttpServerRequestEventEditor & override;

private:
    HttpServerRequest &_request; ///< Owning request.
};

}
