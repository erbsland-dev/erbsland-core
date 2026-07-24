// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../core/Definitions.hpp"

#include <cstdint>

namespace erbsland::network {

/// The version of an IP address.
enum class IpVersion : uint8_t {
    V4, ///< Internet Protocol version 4.
    V6, ///< Internet Protocol version 6.
};

}
