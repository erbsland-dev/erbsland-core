// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

namespace erbsland::conf::impl::placeholder {

/// Semantic identifiers for the built-in text-filter parameters.
enum class TextPlaceholderParameterKey : int {
    Start,
    Length,
    Side,
    Chars,
    Text,
    Replacement,
    Format,
    Amount,
    Contains,
    LengthEqual,
    LengthGreater,
    LengthLess,
    Empty,
    Then,
    Else,
    Positional,
};

}
