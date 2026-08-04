// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "IntegerUnitIndex.hpp"
#include "VersionUnit.hpp"

namespace erbsland::unit {

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
using Major = IntegerUnitIndex<MajorUnit>;
/// A minor version number.
using Minor = IntegerUnitIndex<MinorUnit>;
/// A revision version number.
using Revision = IntegerUnitIndex<RevisionUnit>;
/// A build version number.
using BuildNumber = IntegerUnitIndex<BuildNumberUnit>;

}
