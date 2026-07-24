// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../core/Definitions.hpp"

#include <cstdint>

namespace erbsland::cryptology {

/// A coarse security level for selecting cryptographic algorithms.
/// The values describe relative library policy and do not promise safety for a particular time horizon.
/// @seedoc{/reference/cryptology/hashing}
enum class CryptographicSecurity : uint8_t {
    Standard, ///< The standard security level for general-purpose use.
    High,     ///< A larger security margin for high-value or long-lived results.
};

}
