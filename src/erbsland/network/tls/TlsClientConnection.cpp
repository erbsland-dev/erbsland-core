// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TlsClientConnection.hpp"

namespace erbsland::network {

void TlsClientConnection::connect(HostEndpoint endpoint) {
    connect(std::move(endpoint), {});
}

}
