// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../mem/ByteBlock.hpp"

namespace erbsland::data::asn1 {

/// Decoded BIT STRING content sharing its source storage.
/// @tested{Asn1NodeTest}
struct Asn1BitString final {
    mem::ByteBlock bytes; ///< Value octets following the unused-bit count.
    uint8_t unusedBits{}; ///< Number of low-order unused bits in the final octet.
};

}
