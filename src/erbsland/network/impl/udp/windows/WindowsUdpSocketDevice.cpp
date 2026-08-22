// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "WindowsUdpSocketDevice.hpp"

#include "WindowsUdpSocketState.hpp"

#include <memory>
#include <utility>

namespace erbsland::network::impl {

WindowsUdpSocketDevice::WindowsUdpSocketDevice(event::EventLoopDriverPtr driver, UdpSocketDeviceCallbacks callbacks) :
    _state{std::make_shared<WindowsUdpSocketState>(std::move(driver), std::move(callbacks))} {
}

WindowsUdpSocketDevice::~WindowsUdpSocketDevice() {
    close();
}

auto WindowsUdpSocketDevice::bind(IpEndpoint localEndpoint, const unit::ByteLength maximumDatagramSize) -> IpEndpoint {
    return _state->bind(std::move(localEndpoint), maximumDatagramSize);
}

auto WindowsUdpSocketDevice::send(const UdpDatagram &datagram) -> UdpSocketDeviceSendStatus {
    return _state->send(datagram);
}

void WindowsUdpSocketDevice::setReceiving(const bool enabled) {
    _state->setReceiving(enabled);
}

void WindowsUdpSocketDevice::close() noexcept {
    _state->close();
}

void WindowsUdpSocketDevice::abort() noexcept {
    _state->close();
}

auto UdpSocketDevice::createUdpSocketDevice(event::EventLoopDriverPtr driver, UdpSocketDeviceCallbacks callbacks)
    -> UdpSocketDevicePtr {
    return std::make_unique<WindowsUdpSocketDevice>(std::move(driver), std::move(callbacks));
}

}
