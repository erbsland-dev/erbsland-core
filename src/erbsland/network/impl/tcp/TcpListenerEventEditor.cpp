// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TcpListenerEventEditor.hpp"

#include "TcpListener.hpp"

namespace erbsland::network::impl {

TcpListenerEventEditor::TcpListenerEventEditor(event::EventSourcePtr source, event::EventsPtr target) :
    network::TcpListenerEventEditor{std::move(source), std::move(target)} {
}

auto TcpListenerEventEditor::onListening(NetworkEventFn callback) -> network::TcpListenerEventEditor & {
    listener()->_onListening = std::move(callback);
    return *this;
}

auto TcpListenerEventEditor::onConnection(TcpConnectionRequestFn callback) -> network::TcpListenerEventEditor & {
    listener()->_onConnection = std::move(callback);
    return *this;
}

auto TcpListenerEventEditor::onClosed(NetworkEventFn callback) -> network::TcpListenerEventEditor & {
    listener()->_onClosed = std::move(callback);
    return *this;
}

auto TcpListenerEventEditor::onError(NetworkErrorFn callback) -> network::TcpListenerEventEditor & {
    listener()->_onError = std::move(callback);
    return *this;
}

auto TcpListenerEventEditor::onFinal(NetworkEventFn callback) -> network::TcpListenerEventEditor & {
    listener()->_onFinal = std::move(callback);
    return *this;
}

auto TcpListenerEventEditor::listener() const -> std::shared_ptr<TcpListener> {
    return std::static_pointer_cast<TcpListener>(source());
}

}
