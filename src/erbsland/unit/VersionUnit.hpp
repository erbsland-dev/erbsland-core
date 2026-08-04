// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "IntegerUnit.hpp"
#include "VersionPart.hpp"

#include <cstdint>

namespace erbsland::unit {

/// The integer unit for version components.
/// @tested{VersionTest}
struct VersionUnit : IntegerUnit {
    using IndexType = uint16_t;                ///< The unsigned integer type for version parts.
    using AmountType = uint16_t;               ///< The unsigned integer type for version-part movements.
    using OffsetType = int32_t;                ///< The signed integer type for version-part offsets.
    static constexpr auto cHasNoIndex = false; ///< Version parts use the full 16-bit range.
};

}

#include "VersionUnitTypes.hpp"

#include "impl/VersionComponentTraits.hpp"
