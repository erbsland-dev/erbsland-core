// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

namespace erbsland::text::impl {

template <typename tLength, typename tIndex>
auto StringPatternData::lengthFromStart(const tIndex index) noexcept -> tLength {
    return index.distanceFromZero();
}

template <typename tLength, typename tIndex>
auto StringPatternData::lengthToEnd(const tLength length, const tIndex index) noexcept -> tLength {
    return length - index.distanceFromZero();
}

template <typename tString>
auto StringPatternData::match(const StringPatternView &patternView, const tString &text) const noexcept
    -> MatchResult<tString> {
    if (patternView.kind != StringPatternKind::Divider) {
        return matchGlob(patternView, text);
    }
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

template <typename tString>
auto StringPatternData::matchGlob(const StringPatternView &patternView, const tString &text) const noexcept
    -> MatchResult<tString> {
    auto position = text.indexAt(StringSide::Front);
    const auto end = text.indexAt(StringSide::Back);
    auto retry = position;
    auto element = std::size_t{};
    auto star = cNoStringPatternDivider;
    const auto elements = patternView.elements;
    while (position < end) {
        if (element < elements.size() && elements[element].kind == StringPatternElementKind::AnyText) {
            star = ++element;
            retry = position;
            continue;
        }
        auto next = position;
        if (element < elements.size() &&
            matchesElement(patternView, elements[element], text.readCharAndAdvance(next))) {
            position = next;
            ++element;
            continue;
        }
        if (star == cNoStringPatternDivider) {
            return {};
        }
        element = star;
        if (text.readCharAndAdvance(retry).isSignal()) {
            return {};
        }
        position = retry;
    }
    while (element < elements.size() && elements[element].kind == StringPatternElementKind::AnyText) {
        ++element;
    }
    return {.matched = element == elements.size(), .frontEnd = end, .suffixStart = end};
}

}
