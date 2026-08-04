// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::cterm::impl::document_renderer {

/// The concrete kind of a prepared document block.
enum class BlockKind : uint8_t {
    Paragraph,
    Preformatted,
    FilledLine,
    HorizontalRule,
};

}
