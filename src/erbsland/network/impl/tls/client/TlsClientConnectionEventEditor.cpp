// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TlsClientConnectionEventEditor.hpp"

#include "TlsClientConnection.hpp"

namespace erbsland::network::impl {

TlsClientConnectionEventEditor::TlsClientConnectionEventEditor(event::EventSourcePtr source, event::EventsPtr target) :
    network::TlsClientConnectionEventEditor{std::move(source), std::move(target)} {
}

auto TlsClientConnectionEventEditor::onHostResolved(TcpHostResolvedFn callback)
    -> network::TlsClientConnectionEventEditor & {
    connection()->_onHostResolved = std::move(callback);
    return *this;
}

auto TlsClientConnectionEventEditor::onTransportConnected(NetworkEventFn callback)
    -> network::TlsClientConnectionEventEditor & {
    connection()->_onTransportConnected = std::move(callback);
    return *this;
}

auto TlsClientConnectionEventEditor::onPeerHello(NetworkEventFn callback) -> network::TlsClientConnectionEventEditor & {
    connection()->_onPeerHello = std::move(callback);
    return *this;
}

auto TlsClientConnectionEventEditor::onPeerAuthenticated(NetworkEventFn callback)
    -> network::TlsClientConnectionEventEditor & {
    connection()->_onPeerAuthenticated = std::move(callback);
    return *this;
}

auto TlsClientConnectionEventEditor::onHandshakeCompleted(NetworkEventFn callback)
    -> network::TlsClientConnectionEventEditor & {
    connection()->_onHandshakeCompleted = std::move(callback);
    return *this;
}

auto TlsClientConnectionEventEditor::onData(NetworkDataFn callback) -> network::TlsClientConnectionEventEditor & {
    connection()->_onData = std::move(callback);
    return *this;
}

auto TlsClientConnectionEventEditor::onWritable(NetworkEventFn callback) -> network::TlsClientConnectionEventEditor & {
    connection()->_onWritable = std::move(callback);
    return *this;
}

auto TlsClientConnectionEventEditor::onClosed(ConnectionCloseFn callback) -> network::TlsClientConnectionEventEditor & {
    connection()->_onClosed = std::move(callback);
    return *this;
}

auto TlsClientConnectionEventEditor::onError(NetworkErrorFn callback) -> network::TlsClientConnectionEventEditor & {
    connection()->_onError = std::move(callback);
    return *this;
}

auto TlsClientConnectionEventEditor::onFinal(NetworkEventFn callback) -> network::TlsClientConnectionEventEditor & {
    connection()->_onFinal = std::move(callback);
    return *this;
}

auto TlsClientConnectionEventEditor::connection() const -> std::shared_ptr<TlsClientConnection> {
    return std::static_pointer_cast<TlsClientConnection>(source());
}

}
