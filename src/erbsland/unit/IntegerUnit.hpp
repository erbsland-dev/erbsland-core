// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::unit {

/// The base class for all integer units.
struct IntegerUnit {
    using IndexType = uint64_t;               ///< The unsigned integer type for indexes.
    using AmountType = uint64_t;              ///< The unsigned integer type for amounts.
    using OffsetType = int64_t;               ///< The signed integer type for offsets.
    static constexpr auto cHasNoIndex = true; ///< Whether indexes reserve a no-index state.
};

}
