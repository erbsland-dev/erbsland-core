// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "UdpDatagramDropReason_fwd.hpp"

#include "../../core/Definitions.hpp"

#include <cstdint>

namespace erbsland::network {

/// The reason why an observed incoming UDP datagram was discarded.
enum class UdpDatagramDropReason : std::uint8_t {
    TooLarge, ///< The payload exceeded the configured maximum datagram size.
};

}
