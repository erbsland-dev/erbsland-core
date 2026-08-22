// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TlsServerConnectionEventEditor.hpp"

#include "TlsServerConnection.hpp"

namespace erbsland::network::impl {

TlsServerConnectionEventEditor::TlsServerConnectionEventEditor(event::EventSourcePtr source, event::EventsPtr target) :
    network::TlsServerConnectionEventEditor{std::move(source), std::move(target)} {
}

auto TlsServerConnectionEventEditor::onTransportConnected(NetworkEventFn callback)
    -> network::TlsServerConnectionEventEditor & {
    connection()->_onTransportConnected = std::move(callback);
    return *this;
}

auto TlsServerConnectionEventEditor::onClientHello(NetworkEventFn callback)
    -> network::TlsServerConnectionEventEditor & {
    connection()->_onClientHello = std::move(callback);
    return *this;
}

auto TlsServerConnectionEventEditor::onHandshakeCompleted(NetworkEventFn callback)
    -> network::TlsServerConnectionEventEditor & {
    connection()->_onHandshakeCompleted = std::move(callback);
    return *this;
}

auto TlsServerConnectionEventEditor::onData(NetworkDataFn callback) -> network::TlsServerConnectionEventEditor & {
    connection()->_onData = std::move(callback);
    return *this;
}

auto TlsServerConnectionEventEditor::onWritable(NetworkEventFn callback) -> network::TlsServerConnectionEventEditor & {
    connection()->_onWritable = std::move(callback);
    return *this;
}

auto TlsServerConnectionEventEditor::onClosed(ConnectionCloseFn callback) -> network::TlsServerConnectionEventEditor & {
    connection()->_onClosed = std::move(callback);
    return *this;
}

auto TlsServerConnectionEventEditor::onError(NetworkErrorFn callback) -> network::TlsServerConnectionEventEditor & {
    connection()->_onError = std::move(callback);
    return *this;
}

auto TlsServerConnectionEventEditor::onFinal(NetworkEventFn callback) -> network::TlsServerConnectionEventEditor & {
    connection()->_onFinal = std::move(callback);
    return *this;
}

auto TlsServerConnectionEventEditor::connection() const -> std::shared_ptr<TlsServerConnection> {
    return std::static_pointer_cast<TlsServerConnection>(source());
}

}
