// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../util/EnumFlags.hpp"

#include <cstdint>

namespace erbsland::path {

/// Select syntax accepted when compiling a path glob.
enum class PathGlobPatternFlag : std::uint8_t {
    None = 0U,                                    ///< Disable every optional syntax feature.
    RecursiveWildcard = 1U << 0U,                 ///< Permit whole-component **.
    TrimWhitespace = 1U << 1U,                    ///< Trim outer bare spaces and tabs from each alternative.
    MultiplePatterns = 1U << 2U,                  ///< Permit newline-separated alternatives.
    SemicolonSeparator = 1U << 3U,                ///< Permit semicolon separators; requires MultiplePatterns.
    IgnoreComments = 1U << 4U,                    ///< Ignore lines starting with # after optional whitespace trimming.
    Default = RecursiveWildcard | TrimWhitespace, ///< Default single-pattern syntax.
};
/// Flags controlling path-glob compilation.
using PathGlobPatternFlags = util::EnumFlags<PathGlobPatternFlag>;

}
