// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../CharSet_fwd.hpp"

namespace erbsland::text::impl {

/// Get the shared ASCII whitespace set for string transformations.
/// @return Space and the ASCII tab-through-carriage-return range.
/// @tested{StringTransformationsTest}
[[nodiscard]] auto asciiTransformationSeparators() -> const CharSet &;

/// Get the shared Unicode whitespace set for string transformations.
/// @return ASCII whitespace, U+0085, and all Unicode separator categories.
/// @tested{StringTransformationsTest}
[[nodiscard]] auto unicodeTransformationSeparators() -> const CharSet &;

}
