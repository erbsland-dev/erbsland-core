// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <memory>

namespace erbsland::network {

class TcpListener;

/// A shared TCP listener.
using TcpListenerPtr = std::shared_ptr<TcpListener>;

}
