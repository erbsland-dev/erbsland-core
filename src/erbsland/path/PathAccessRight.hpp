// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../util/EnumFlags.hpp"

#include <cstdint>

namespace erbsland::path {

/// Portable access rights for files and directories.
enum class PathAccessRight : uint8_t {
    None = 0U,                    ///< No access rights.
    Read = 1U << 0U,              ///< The object can be read.
    Write = 1U << 1U,             ///< The object can be modified.
    Execute = 1U << 2U,           ///< File execution or directory traversal/search.
    All = Read | Write | Execute, ///< All portable rights.
};

/// A set of portable access rights.
using PathAccessRights = util::EnumFlags<PathAccessRight>;

}
