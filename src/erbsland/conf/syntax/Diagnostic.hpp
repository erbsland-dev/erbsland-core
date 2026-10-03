// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../ConfErrorCategory.hpp"

#include "../../unit/ByteRange.hpp"
#include "../../unit/CodeLocation.hpp"

namespace erbsland::conf::syntax {

/// A source diagnostic; ranges always refer to the original input.
/// @tested{SyntaxParserTest}
struct Diagnostic {
    ConfErrorCategory category;  ///< The failure category.
    text::String message;        ///< Human-readable description.
    unit::ByteRange range;       ///< Affected bytes, or an empty range for missing input.
    unit::CodeLocation location; ///< Original source location.
    bool warning{false};         ///< Whether this diagnostic is advisory.
};

}
