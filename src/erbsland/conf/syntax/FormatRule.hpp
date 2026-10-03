// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "FormatStylePatch.hpp"
#include "Selector.hpp"

namespace erbsland::conf::syntax {

/// A selector and the properties it contributes to the cascade.
/// @tested{SyntaxFormatTest}
struct FormatRule {
    Selector selector;      ///< Statement selection.
    FormatStylePatch style; ///< Explicit overrides.
};

}
