// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TcpConnectionEventEditor.hpp"

#include "TcpConnection.hpp"

namespace erbsland::network::impl {

TcpConnectionEventEditor::TcpConnectionEventEditor(event::EventSourcePtr source, event::EventsPtr target) :
    network::TcpConnectionEventEditor{std::move(source), std::move(target)} {
}

auto TcpConnectionEventEditor::onHostResolved(TcpHostResolvedFn callback) -> network::TcpConnectionEventEditor & {
    connection()->_onHostResolved = std::move(callback);
    return *this;
}

auto TcpConnectionEventEditor::onConnected(NetworkEventFn callback) -> network::TcpConnectionEventEditor & {
    connection()->_onConnected = std::move(callback);
    return *this;
}

auto TcpConnectionEventEditor::onData(NetworkDataFn callback) -> network::TcpConnectionEventEditor & {
    connection()->_onData = std::move(callback);
    return *this;
}

auto TcpConnectionEventEditor::onWritable(NetworkEventFn callback) -> network::TcpConnectionEventEditor & {
    connection()->_onWritable = std::move(callback);
    return *this;
}

auto TcpConnectionEventEditor::onClosed(TcpConnectionCloseFn callback) -> network::TcpConnectionEventEditor & {
    connection()->_onClosed = std::move(callback);
    return *this;
}

auto TcpConnectionEventEditor::onError(NetworkErrorFn callback) -> network::TcpConnectionEventEditor & {
    connection()->_onError = std::move(callback);
    return *this;
}

auto TcpConnectionEventEditor::onFinal(NetworkEventFn callback) -> network::TcpConnectionEventEditor & {
    connection()->_onFinal = std::move(callback);
    return *this;
}

auto TcpConnectionEventEditor::connection() const -> std::shared_ptr<TcpConnection> {
    return std::static_pointer_cast<TcpConnection>(source());
}

}
