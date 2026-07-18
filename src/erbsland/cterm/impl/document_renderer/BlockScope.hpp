// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ListItemLayout.hpp"

#include "../../../bgeo/BlockMargins.hpp"
#include "../../BlockStringEditor.hpp"

#include <optional>

namespace erbsland::cterm::impl::document_renderer {

/// State kept for one open block-rendering scope.
/// @tested{TerminalDocumentRendererTest}
struct BlockScope final {
    bgeo::BlockMargins margins{0};                ///< The scope margins around its content.
    std::optional<ListItemLayout> listItemLayout; ///< Optional active list-item layout.
    bool hasBlocks{false};                        ///< `true` after the first block in this scope.
    std::optional<BlockString> linePrefix;        ///< Prefix applied to content lines.
};

}
