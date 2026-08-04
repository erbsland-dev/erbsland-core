// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TcpListenerOptions.hpp"

namespace erbsland::network {

TcpListenerOptions::TcpListenerOptions() : _connectionQuota{ConnectionQuota::create(cDefaultMaximumConnections)} {
}

}
