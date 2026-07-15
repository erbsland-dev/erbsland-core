// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "IntegerUnit.hpp"

namespace erbsland::unit {

/// The integer unit for source code line types.
/// @tested{IntegerUnitTest}
struct LineUnit final : IntegerUnit {
    using IndexType = uint32_t;  ///< The unsigned integer type for indexes.
    using AmountType = uint32_t; ///< The unsigned integer type for amounts.
    using OffsetType = int32_t;  ///< The signed integer type for offsets.
};

}
