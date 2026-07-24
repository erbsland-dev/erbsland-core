// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../core/Definitions.hpp"

#include <cstdint>

namespace erbsland::path {

/// Portable access profile for creating or changing files and directories.
enum class PathAccessProfile : uint8_t {
    Default,      ///< Use platform defaults.
    UserOnly,     ///< Only the current user should have regular access.
    UserAndGroup, ///< The current user and group should have regular access.
    Everyone,     ///< Everyone should have regular access.
};

}
