// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::cterm::impl::paragraph {

/// How newline characters in the input string are interpreted by the paragraph layout.
enum class LayoutNewlineMode : uint8_t {
    ParagraphBreak, ///< Each newline starts a new paragraph.
    HardLineBreak,  ///< Each newline inserts a hard line break inside one paragraph.
};

}
