// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CodeSnippetLayoutCell.hpp"

#include <vector>

namespace erbsland::text::impl {

/// One physical source row without a gutter or marker annotation.
struct CodeSnippetLayoutRow final {
    std::vector<CodeSnippetLayoutCell> cells; ///< The visible source cells.
    bool hasLeadingEllipsis{false};           ///< `true` if source content was omitted before this row.
    bool hasTrailingEllipsis{false};          ///< `true` if source content was omitted after this row.
};

}
