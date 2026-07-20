// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CryptographicSecurity.hpp"
#include "CryptographicStatus.hpp"
#include "HashThroughput.hpp"

namespace erbsland::cryptology {

/// Requirements for selecting a hash algorithm.
/// The defaults request an acceptable general-purpose algorithm without imposing a throughput minimum.
/// @seedoc{/reference/cryptology/hashing}
/// @tested{HashAlgorithmTest}
struct HashRequirements final {
    CryptographicStatus requiredStatus{CryptographicStatus::Acceptable};    ///< The exact required status.
    CryptographicSecurity minimumSecurity{CryptographicSecurity::Standard}; ///< The minimum security level.
    HashThroughput minimumThroughput{HashThroughput::Low};                  ///< The minimum relative hash throughput.
};

}
