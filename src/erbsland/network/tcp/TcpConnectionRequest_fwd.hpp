// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <memory>

namespace erbsland::network {

class TcpConnectionRequest;

/// A shared pending incoming TCP connection request.
using TcpConnectionRequestPtr = std::shared_ptr<TcpConnectionRequest>;

}
