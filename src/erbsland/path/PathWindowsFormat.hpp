// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../core/Definitions.hpp"

#include <cstdint>

namespace erbsland::path {

/// The format in which a Windows path is converted.
enum class PathWindowsFormat : std::uint8_t {
    /// Returns the path using backslash (`\`) path separators.
    Native,
    /// Returns the path using backslash (`\`) path separators as an extended length path.
    /// This will add the Windows extended-length prefix for regular paths or UNC paths.
    Extended,
};

}
