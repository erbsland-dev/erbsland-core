// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "InlineContent.hpp"

namespace erbsland::cterm::impl::document_renderer {

void InlineContent::append(const BlockStringView text) {
    _text += text;
}

void InlineContent::append(const InlineContent &content) {
    const auto offset = _text.length();
    _text += content.text();
    for (const auto breakIndex : content.semantics().softBreaks) {
        _semantics.softBreaks.push_back(breakIndex.advanced(offset));
    }
    for (const auto &range : content.semantics().indivisibleRanges) {
        _semantics.indivisibleRanges.push_back(range.advanced(offset));
    }
}

void InlineContent::addSoftBreak() {
    const auto index = BlockIndex::end(_text.length());
    if (_semantics.softBreaks.empty() || _semantics.softBreaks.back() != index) {
        _semantics.softBreaks.push_back(index);
    }
}

void InlineContent::addIndivisibleRange(const BlockRange range) {
    if (!range.isEmpty()) {
        _semantics.indivisibleRanges.push_back(range);
    }
}

}
