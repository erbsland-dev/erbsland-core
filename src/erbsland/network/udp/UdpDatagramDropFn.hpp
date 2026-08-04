// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "UdpDatagramDropContext_fwd.hpp"

#include <functional>

namespace erbsland::network {

/// A callback receiving details about one locally discarded UDP datagram.
using UdpDatagramDropFn = std::function<void(const UdpDatagramDropContext &)>;

}
