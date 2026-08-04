// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TcpListener.hpp"

namespace erbsland::network {

void TcpListener::start(IpEndpoint localEndpoint) {
    start(std::move(localEndpoint), {});
}

}
