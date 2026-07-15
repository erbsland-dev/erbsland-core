// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ListItemLayout.hpp"

namespace erbsland::cterm::impl::document_renderer {

void ListItemLayout::applyPrefixTo(RenderBlock &block) const {
    auto indents = block.indents();
    const auto baseIndent = indents.lineIndent();
    indents.setFirstLineIndent(baseIndent + firstLineOffset);
    indents.setWrappedLineIndent(baseIndent + continuationOffset);
    block.indents() = indents;
    block.setListPrefix(prefix);
}

void ListItemLayout::applyContinuationTo(RenderBlock &block) const {
    auto indents = block.indents();
    const auto continuationIndent = indents.lineIndent() + continuationOffset;
    indents.setLineIndent(continuationIndent);
    indents.setFirstLineIndent(continuationIndent);
    indents.setWrappedLineIndent(continuationIndent);
    block.indents() = indents;
}

}
