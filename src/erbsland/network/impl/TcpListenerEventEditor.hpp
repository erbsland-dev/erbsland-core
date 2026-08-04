// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TcpListener_fwd.hpp"

#include "../tcp/TcpListenerEventEditor.hpp"

namespace erbsland::network::impl {

/// Event editor for the native TCP listener implementation.
/// @tested{TcpListenerTest TcpSocketLiveTest}
class TcpListenerEventEditor final : public network::TcpListenerEventEditor {
public:
    /// Create the source-owned editor.
    TcpListenerEventEditor(event::EventSourcePtr source, event::EventsPtr target);

public: // implement network::TcpListenerEventEditor
    auto onListening(NetworkEventFn callback) -> network::TcpListenerEventEditor & override;
    auto onConnection(TcpConnectionRequestFn callback) -> network::TcpListenerEventEditor & override;
    auto onClosed(NetworkEventFn callback) -> network::TcpListenerEventEditor & override;
    auto onError(NetworkErrorFn callback) -> network::TcpListenerEventEditor & override;
    auto onFinal(NetworkEventFn callback) -> network::TcpListenerEventEditor & override;

private:
    /// Lock and return the TCP listener that owns this editor.
    [[nodiscard]] auto listener() const -> std::shared_ptr<TcpListener>;
};

}
