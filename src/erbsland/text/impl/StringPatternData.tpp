// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

namespace erbsland::text::impl {

template <typename tLength, typename tIndex>
auto StringPatternData::lengthFromStart(const tIndex index) noexcept -> tLength {
    return tLength{static_cast<typename tLength::Value>(index.toRawValue())};
}

template <typename tLength, typename tIndex>
auto StringPatternData::lengthToEnd(const tLength length, const tIndex index) noexcept -> tLength {
    return tLength{static_cast<typename tLength::Value>(length.toRawValue() - index.toRawValue())};
}

template <typename tString>
auto StringPatternData::match(const StringPatternView &patternView, const tString &text) const noexcept
    -> MatchResult<tString> {
    auto result = MatchResult<tString>{};
    result.frontEnd = text.indexAt(StringSide::Front);
    result.suffixStart = text.indexAt(StringSide::Back);
    if (patternView.elements.empty() && patternView.divider == cNoStringPatternDivider) {
        return result;
    }
    if (patternView.divider == cNoStringPatternDivider) {
        if (!matchFront(patternView, text, 0U, patternView.elements.size(), result.frontEnd)) {
            return result;
        }
        result.matched = true;
        return result;
    }
    const auto frontCount = patternView.divider;
    const auto backCount = patternView.elements.size() - patternView.divider;
    if (frontCount == 0U && backCount == 0U) {
        return result;
    }
    if (!matchFront(patternView, text, 0U, patternView.divider, result.frontEnd)) {
        return result;
    }
    if (!matchBack(patternView, text, patternView.divider, patternView.elements.size(), result.suffixStart)) {
        return result;
    }
    if (result.frontEnd > result.suffixStart) {
        return result;
    }
    result.matched = true;
    return result;
}

template <typename tString, typename tIndex>
auto StringPatternData::matchFront(
    const StringPatternView &patternView,
    const tString &text,
    const std::size_t begin,
    const std::size_t end,
    tIndex &index) const noexcept -> bool {
    index = text.indexAt(StringSide::Front);
    for (auto elementIndex = begin; elementIndex < end; ++elementIndex) {
        if (!matchesElement(patternView, patternView.elements[elementIndex], text.readCharAndAdvance(index))) {
            return false;
        }
    }
    return true;
}

template <typename tString, typename tIndex>
auto StringPatternData::matchBack(
    const StringPatternView &patternView,
    const tString &text,
    const std::size_t begin,
    const std::size_t end,
    tIndex &index) const noexcept -> bool {
    index = text.indexAt(StringSide::Back);
    for (auto elementIndex = end; elementIndex > begin; --elementIndex) {
        if (!matchesElement(patternView, patternView.elements[elementIndex - 1U], text.readCharAndRetreat(index))) {
            return false;
        }
    }
    return true;
}

}
