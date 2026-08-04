// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TcpConnection_fwd.hpp"

#include "../tcp/TcpConnectionEventEditor.hpp"

namespace erbsland::network::impl {

/// Event editor for the native TCP connection implementation.
/// @tested{TcpConnectionTest TcpSocketLiveTest}
class TcpConnectionEventEditor final : public network::TcpConnectionEventEditor {
public:
    /// Create the source-owned editor.
    TcpConnectionEventEditor(event::EventSourcePtr source, event::EventsPtr target);

public: // implement network::TcpConnectionEventEditor
    auto onHostResolved(TcpHostResolvedFn callback) -> network::TcpConnectionEventEditor & override;
    auto onConnected(NetworkEventFn callback) -> network::TcpConnectionEventEditor & override;
    auto onData(NetworkDataFn callback) -> network::TcpConnectionEventEditor & override;
    auto onWritable(NetworkEventFn callback) -> network::TcpConnectionEventEditor & override;
    auto onClosed(TcpConnectionCloseFn callback) -> network::TcpConnectionEventEditor & override;
    auto onError(NetworkErrorFn callback) -> network::TcpConnectionEventEditor & override;
    auto onFinal(NetworkEventFn callback) -> network::TcpConnectionEventEditor & override;

private:
    /// Lock and return the TCP connection that owns this editor.
    [[nodiscard]] auto connection() const -> std::shared_ptr<TcpConnection>;
};

}
