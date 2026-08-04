// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TcpConnection.hpp"

#include "TcpAcceptOptions.hpp"
#include "TcpConnectOptions.hpp"

namespace erbsland::network {

void TcpConnection::connect(HostEndpoint remoteEndpoint) {
    connect(std::move(remoteEndpoint), {});
}

void TcpConnection::accept(TcpConnectionRequestPtr request) {
    accept(std::move(request), {});
}

}
