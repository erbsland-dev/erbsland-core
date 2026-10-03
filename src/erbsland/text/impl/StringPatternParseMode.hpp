// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::text::impl {

/// Select the syntax and stopping boundary of the shared pattern parser.
enum class StringPatternParseMode : std::uint8_t {
    Divider,         ///< Parse a complete prefix/divider pattern.
    Glob,            ///< Parse a complete string glob.
    PartialPathGlob, ///< Parse one path component, leaving its delimiter unread.
};

}
