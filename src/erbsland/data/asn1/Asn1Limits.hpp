// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../unit/ByteCount.hpp"
#include "../../unit/ItemCount.hpp"

namespace erbsland::data::asn1 {

/// Resource budgets for one encoded document.
/// @tested{BerCodecTest}
struct Asn1Limits final {
    unit::ByteCount maximumLength{16U * 1024U * 1024U}; ///< Maximum total encoded length.
    unit::ItemCount maximumDepth{32U};   ///< Maximum levels, including the root. The implementation supports up to 32.
    unit::ItemCount maximumNodes{8192U}; ///< Maximum number of values, including containers.
};

}
