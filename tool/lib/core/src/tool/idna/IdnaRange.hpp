// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

namespace tool::idna {

/// One compact runtime attribute range, confined to a 16-bit Unicode page.
/// @tested{IdnaDataFullTest}
struct IdnaRange {
    char32_t first{};      ///< First code point, inclusive.
    char32_t last{};       ///< Last code point, inclusive.
    unsigned attributes{}; ///< Thirteen-bit runtime attributes.
};

}
