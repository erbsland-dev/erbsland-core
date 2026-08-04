// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "UdpSocket.hpp"

namespace erbsland::network {

void UdpSocket::start(UdpSocketOptions options) {
    start(IpEndpoint{}, std::move(options));
}

void UdpSocket::start(IpAddress localAddress, UdpSocketOptions options) {
    start(IpEndpoint{std::move(localAddress), Port{}}, std::move(options));
}

void UdpSocket::start(const Port localPort, UdpSocketOptions options) {
    start(IpEndpoint{IpAddress::anyV4(), localPort}, std::move(options));
}

void UdpSocket::start(IpAddress localAddress, const Port localPort, UdpSocketOptions options) {
    start(IpEndpoint{std::move(localAddress), localPort}, std::move(options));
}

auto UdpSocket::send(const IpEndpoint &destination, const mem::ByteBlock &data) -> NetworkSendStatus {
    return send(UdpDatagram{destination, data});
}

}
