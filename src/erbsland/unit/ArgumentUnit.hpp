// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "IntegerUnit.hpp"
#include "IntegerUnitAmount.hpp"
#include "IntegerUnitIndex.hpp"

namespace erbsland::unit {

/// The integer unit for argument types.
/// @tested{IntegerUnitTest}
struct ArgumentUnit final : IntegerUnit {
    using IndexType = uint32_t;  ///< The unsigned integer type for indexes.
    using AmountType = uint32_t; ///< The unsigned integer type for amounts.
    using OffsetType = int32_t;  ///< The signed integer type for offsets.
};

/// An argument index.
using ArgumentIndex = IntegerUnitIndex<ArgumentUnit>;
/// An argument count.
using ArgumentCount = IntegerUnitAmount<ArgumentUnit>;

}
