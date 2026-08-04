// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <memory>

namespace erbsland::network {

class TcpConnection;

/// A shared connected TCP stream.
using TcpConnectionPtr = std::shared_ptr<TcpConnection>;

}
