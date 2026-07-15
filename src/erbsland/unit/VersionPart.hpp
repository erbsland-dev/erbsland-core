// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::unit {

/// The part of a version number.
///
/// The order of the values follows their significance in a version.
///
enum class VersionPart : uint8_t {
    Major,    ///< The major version part.
    Minor,    ///< The minor version part.
    Revision, ///< The revision version part.
    Build,    ///< The build version part.
};

}
