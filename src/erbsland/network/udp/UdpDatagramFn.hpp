// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "UdpDatagram.hpp"

#include <functional>

namespace erbsland::network {

/// A callback receiving an owned UDP datagram.
using UdpDatagramFn = std::function<void(UdpDatagram)>;

}
