// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../util/EnumFlags.hpp"

#include <cstdint>

namespace erbsland::path {

/// Common native file attributes that are not covered by portable access rights.
enum class PathAttribute : uint8_t {
    None = 0U,                                              ///< No attributes.
    ReadOnly = 1U << 0U,                                    ///< Windows read-only attribute.
    Immutable = 1U << 1U,                                   ///< POSIX/macOS immutable flag, where supported.
    Hidden = 1U << 2U,                                      ///< Native hidden attribute, where supported.
    Archive = 1U << 3U,                                     ///< Windows archive attribute.
    System = 1U << 4U,                                      ///< Windows system attribute.
    All = ReadOnly | Immutable | Hidden | Archive | System, ///< All known native attributes.
};

/// A set of native file attributes.
using PathAttributes = util::EnumFlags<PathAttribute>;

}
