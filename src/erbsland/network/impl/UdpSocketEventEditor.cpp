// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "UdpSocketEventEditor.hpp"

#include "UdpSocket.hpp"

namespace erbsland::network::impl {

UdpSocketEventEditor::UdpSocketEventEditor(event::EventSourcePtr source, event::EventsPtr target) :
    network::UdpSocketEventEditor{std::move(source), std::move(target)} {
}

auto UdpSocketEventEditor::onBound(NetworkEventFn callback) -> network::UdpSocketEventEditor & {
    socket()->_onBound = std::move(callback);
    return *this;
}

auto UdpSocketEventEditor::onDatagram(UdpDatagramFn callback) -> network::UdpSocketEventEditor & {
    socket()->_onDatagram = std::move(callback);
    return *this;
}

auto UdpSocketEventEditor::onDatagramDropped(UdpDatagramDropFn callback) -> network::UdpSocketEventEditor & {
    socket()->_onDatagramDropped = std::move(callback);
    return *this;
}

auto UdpSocketEventEditor::onWritable(NetworkEventFn callback) -> network::UdpSocketEventEditor & {
    socket()->_onWritable = std::move(callback);
    return *this;
}

auto UdpSocketEventEditor::onClosed(NetworkEventFn callback) -> network::UdpSocketEventEditor & {
    socket()->_onClosed = std::move(callback);
    return *this;
}

auto UdpSocketEventEditor::onError(NetworkErrorFn callback) -> network::UdpSocketEventEditor & {
    socket()->_onError = std::move(callback);
    return *this;
}

auto UdpSocketEventEditor::socket() const -> std::shared_ptr<UdpSocket> {
    return std::static_pointer_cast<UdpSocket>(source());
}

}
