// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../core/Definitions.hpp"

#include <cstdint>

namespace erbsland::cryptology {

/// The relative throughput of a fixed-output streaming hash implementation.
/// Values compare hashing algorithms in this library and are not comparable to key algorithms or password KDFs.
/// @seedoc{/reference/cryptology/cryptographic_operations}
enum class HashThroughput : uint8_t {
    Low,    ///< The lowest relative hash throughput.
    Medium, ///< A balance between relative hash throughput and security.
    High,   ///< The highest relative hash throughput.
};

}
