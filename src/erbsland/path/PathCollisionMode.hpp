// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../core/Definitions.hpp"

#include <cstdint>

namespace erbsland::path {

/// The mode to use when a path collision is detected for an operation.
enum class PathCollisionMode : uint8_t {
    Stop,      ///< Stop processing the path (with an error)
    Skip,      ///< Skip the path
    Overwrite, ///< Overwrite the existing path
};

}
