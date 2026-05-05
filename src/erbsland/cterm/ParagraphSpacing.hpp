// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::cterm {

/// The spacing between explicit newline-separated paragraphs.
enum class ParagraphSpacing : uint8_t {
    /// Render the next paragraph directly below the previous paragraph.
    SingleLine,
    /// Insert one empty row between paragraphs.
    DoubleLine,
};

}
