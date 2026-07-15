// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../text/CharSet_fwd.hpp"

namespace erbsland::path::impl {

/// The dot character for suffixes.
[[nodiscard]] auto dotCharacters() -> const text::CharSet &;

/// The slash character for paths.
[[nodiscard]] auto slashCharacters() -> const text::CharSet &;

/// The slash and backslash characters for paths.
[[nodiscard]] auto pathSeparators() -> const text::CharSet &;

/// Invalid characters for paths.
/// This is the null character (invalid on all platforms) and the replacement character
/// to catch paths with UTF-8 encoding errors.
[[nodiscard]] auto invalidPathCharacters() -> const text::CharSet &;

}
