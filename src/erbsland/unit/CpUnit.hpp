// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "IntegerUnit.hpp"

#include <cstdint>

namespace erbsland::unit {

/// The integer unit Unicode code-point-based types.
/// @tested{IntegerUnitTest}
struct CpUnit final : IntegerUnit {
    using IndexType = uint32_t;  ///< The unsigned integer type for indexes.
    using AmountType = uint32_t; ///< The unsigned integer type for amounts.
    using OffsetType = int32_t;  ///< The signed integer type for offsets.
};

}
