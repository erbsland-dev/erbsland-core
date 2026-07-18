// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "LayoutLineToken.hpp"

#include "../../BlockString.hpp"

namespace erbsland::cterm::impl::paragraph {

auto LayoutLineToken::remainingWidth(const BlockString &text, const BlockCount offset) const noexcept -> int {
    assert(isWord());
    if (offset.isZero()) {
        return _displayWidth;
    }
    auto width = 0;
    for (auto index = offset; index < _length; ++index) {
        width += text[sourceIndex(index)].displayWidth();
    }
    return width;
}

auto LayoutLineToken::split(
    const BlockString &text,
    const BlockCount offset,
    const int availableWidth,
    const int trailingMarkerWidth) const noexcept -> std::optional<SplitResult> {
    assert(isWord());
    if (availableWidth - trailingMarkerWidth <= 0) {
        return std::nullopt;
    }
    auto result = SplitResult{};
    auto usedWidth = 0;
    for (auto index = offset; index < _length; ++index) {
        const auto characterWidth = text[sourceIndex(index)].displayWidth();
        if (characterWidth <= 0) {
            continue;
        }
        if (usedWidth + characterWidth > availableWidth - trailingMarkerWidth) {
            break;
        }
        usedWidth += characterWidth;
        result.sourceCharacterCount = index - offset + BlockCount::one();
    }
    if (result.sourceCharacterCount.isZero()) {
        return std::nullopt;
    }
    result.sourceWidth = usedWidth;
    result.width = usedWidth;
    if (offset + result.sourceCharacterCount < _length) {
        result.width += trailingMarkerWidth;
    }
    return result;
}

}
