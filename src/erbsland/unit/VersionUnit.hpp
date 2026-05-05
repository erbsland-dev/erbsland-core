// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "IntegerUnit.hpp"
#include "IntegerUnitIndex.hpp"
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

/// The integer unit for major version components.
/// @tested{VersionTest}
struct MajorUnit final : VersionUnit {
    static constexpr auto cPart = VersionPart::Major; ///< The version part represented by this unit.
};

/// The integer unit for minor version components.
/// @tested{VersionTest}
struct MinorUnit final : VersionUnit {
    static constexpr auto cPart = VersionPart::Minor; ///< The version part represented by this unit.
};

/// The integer unit for revision version components.
/// @tested{VersionTest}
struct RevisionUnit final : VersionUnit {
    static constexpr auto cPart = VersionPart::Revision; ///< The version part represented by this unit.
};

/// The integer unit for build-number version components.
/// @tested{VersionTest}
struct BuildNumberUnit final : VersionUnit {
    static constexpr auto cPart = VersionPart::Build; ///< The version part represented by this unit.
};

/// A major version number.
/// @tested{VersionTest}
using Major = IntegerUnitIndex<MajorUnit>;
/// A minor version number.
/// @tested{VersionTest}
using Minor = IntegerUnitIndex<MinorUnit>;
/// A revision version number.
/// @tested{VersionTest}
using Revision = IntegerUnitIndex<RevisionUnit>;
/// A build version number.
/// @tested{VersionTest}
using BuildNumber = IntegerUnitIndex<BuildNumberUnit>;

}

#include "impl/VersionComponentTraits.hpp"
