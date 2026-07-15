// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StringPatternData.hpp"

namespace erbsland::text::impl {

auto StringPatternData::matches(const U8StringView &text) const noexcept -> bool {
    return match(view(), text).matched;
}

auto StringPatternData::matches(const U16StringView &text) const noexcept -> bool {
    return match(view(), text).matched;
}

auto StringPatternData::matches(const U32StringView &text) const noexcept -> bool {
    return match(view(), text).matched;
}

auto StringPatternData::trim(U8StringView &text) const noexcept -> bool {
    const auto trimmedView = trimmed(text);
    if (trimmedView == text) {
        return false;
    }
    text = trimmedView;
    return true;
}

auto StringPatternData::trim(U16StringView &text) const noexcept -> bool {
    const auto trimmedView = trimmed(text);
    if (trimmedView == text) {
        return false;
    }
    text = trimmedView;
    return true;
}

auto StringPatternData::trim(U32StringView &text) const noexcept -> bool {
    const auto trimmedView = trimmed(text);
    if (trimmedView == text) {
        return false;
    }
    text = trimmedView;
    return true;
}

auto StringPatternData::trim(U8String &text) const -> bool {
    const auto textView = U8StringView{text};
    const auto trimmedView = trimmed(textView);
    if (trimmedView == textView) {
        return false;
    }
    text = U8String{trimmedView};
    return true;
}

auto StringPatternData::trim(U16String &text) const -> bool {
    const auto textView = U16StringView{text};
    const auto trimmedView = trimmed(textView);
    if (trimmedView == textView) {
        return false;
    }
    text = U16String{trimmedView};
    return true;
}

auto StringPatternData::trim(U32String &text) const -> bool {
    const auto textView = U32StringView{text};
    const auto trimmedView = trimmed(textView);
    if (trimmedView == textView) {
        return false;
    }
    text = U32String{trimmedView};
    return true;
}

auto StringPatternData::trimmed(const U8StringView &text) const noexcept -> U8StringView {
    const auto patternView = view();
    const auto result = match(patternView, text);
    if (!result.matched) {
        return text;
    }
    if (patternView.divider == cNoStringPatternDivider || patternView.divider == patternView.elements.size()) {
        return text.slice(StringSide::Back, result.frontEnd);
    }
    if (patternView.divider == 0U) {
        return text.slice(StringSide::Front, result.suffixStart);
    }
    return text.slice(unit::ByteRange{result.frontEnd, result.suffixStart});
}

auto StringPatternData::trimmed(const U16StringView &text) const noexcept -> U16StringView {
    const auto patternView = view();
    const auto result = match(patternView, text);
    if (!result.matched) {
        return text;
    }
    if (patternView.divider == cNoStringPatternDivider || patternView.divider == patternView.elements.size()) {
        return text.slice(StringSide::Back, result.frontEnd);
    }
    if (patternView.divider == 0U) {
        return text.slice(StringSide::Front, result.suffixStart);
    }
    return text.slice(unit::U16DataRange{result.frontEnd, result.suffixStart});
}

auto StringPatternData::trimmed(const U32StringView &text) const noexcept -> U32StringView {
    const auto patternView = view();
    const auto result = match(patternView, text);
    if (!result.matched) {
        return text;
    }
    if (patternView.divider == cNoStringPatternDivider || patternView.divider == patternView.elements.size()) {
        return text.slice(StringSide::Back, result.frontEnd);
    }
    if (patternView.divider == 0U) {
        return text.slice(StringSide::Front, result.suffixStart);
    }
    return text.slice(unit::CpRange{result.frontEnd, result.suffixStart});
}

auto StringPatternData::split(const U8StringView &text) const noexcept -> std::pair<U8StringView, U8StringView> {
    const auto patternView = view();
    const auto result = match(patternView, text);
    if (!result.matched) {
        return {U8StringView{}, text};
    }
    if (patternView.divider == 0U) {
        return text.splitAt(result.suffixStart);
    }
    auto [prefix, rest] = text.splitAt(result.frontEnd);
    if (patternView.divider != cNoStringPatternDivider && patternView.divider < patternView.elements.size()) {
        rest = text.slice(unit::ByteRange{result.frontEnd, result.suffixStart});
    }
    return {prefix, rest};
}

auto StringPatternData::split(const U16StringView &text) const noexcept -> std::pair<U16StringView, U16StringView> {
    const auto patternView = view();
    const auto result = match(patternView, text);
    if (!result.matched) {
        return {U16StringView{}, text};
    }
    if (patternView.divider == 0U) {
        return text.splitAt(result.suffixStart);
    }
    auto [prefix, rest] = text.splitAt(result.frontEnd);
    if (patternView.divider != cNoStringPatternDivider && patternView.divider < patternView.elements.size()) {
        rest = text.slice(unit::U16DataRange{result.frontEnd, result.suffixStart});
    }
    return {prefix, rest};
}

auto StringPatternData::split(const U32StringView &text) const noexcept -> std::pair<U32StringView, U32StringView> {
    const auto patternView = view();
    const auto result = match(patternView, text);
    if (!result.matched) {
        return {U32StringView{}, text};
    }
    if (patternView.divider == 0U) {
        return text.splitAt(result.suffixStart);
    }
    auto [prefix, rest] = text.splitAt(result.frontEnd);
    if (patternView.divider != cNoStringPatternDivider && patternView.divider < patternView.elements.size()) {
        rest = text.slice(unit::CpRange{result.frontEnd, result.suffixStart});
    }
    return {prefix, rest};
}

auto StringPatternData::length(const U8StringView &text) const noexcept -> unit::ByteLength {
    const auto patternView = view();
    const auto result = match(patternView, text);
    if (!result.matched) {
        return unit::ByteLength::zero();
    }
    if (patternView.divider == 0U) {
        return lengthToEnd(text.length(), result.suffixStart);
    }
    return lengthFromStart<unit::ByteLength>(result.frontEnd);
}

auto StringPatternData::length(const U16StringView &text) const noexcept -> unit::U16DataLength {
    const auto patternView = view();
    const auto result = match(patternView, text);
    if (!result.matched) {
        return unit::U16DataLength::zero();
    }
    if (patternView.divider == 0U) {
        return lengthToEnd(text.length(), result.suffixStart);
    }
    return lengthFromStart<unit::U16DataLength>(result.frontEnd);
}

auto StringPatternData::length(const U32StringView &text) const noexcept -> unit::CpLength {
    const auto patternView = view();
    const auto result = match(patternView, text);
    if (!result.matched) {
        return unit::CpLength::zero();
    }
    if (patternView.divider == 0U) {
        return lengthToEnd(text.length(), result.suffixStart);
    }
    return lengthFromStart<unit::CpLength>(result.frontEnd);
}

auto StringPatternData::index(const U8StringView &text) const noexcept -> unit::ByteIndex {
    const auto patternView = view();
    const auto result = match(patternView, text);
    if (!result.matched) {
        return unit::ByteIndex::noIndex();
    }
    return patternView.divider == 0U ? result.suffixStart : result.frontEnd;
}

auto StringPatternData::index(const U16StringView &text) const noexcept -> unit::U16DataIndex {
    const auto patternView = view();
    const auto result = match(patternView, text);
    if (!result.matched) {
        return unit::U16DataIndex::noIndex();
    }
    return patternView.divider == 0U ? result.suffixStart : result.frontEnd;
}

auto StringPatternData::index(const U32StringView &text) const noexcept -> unit::CpIndex {
    const auto patternView = view();
    const auto result = match(patternView, text);
    if (!result.matched) {
        return unit::CpIndex::noIndex();
    }
    return patternView.divider == 0U ? result.suffixStart : result.frontEnd;
}

auto StringPatternData::matchesElement(
    const StringPatternView &patternView, const StringPatternElement &element, const Char character) noexcept -> bool {
    if (character.isSignal()) {
        return false;
    }
    switch (element.kind) {
    case StringPatternElementKind::Character:
        return character == element.character;
    case StringPatternElementKind::OneChar:
        return true;
    case StringPatternElementKind::Set: {
        const auto ranges = patternView.ranges.subspan(element.rangeOffset, element.rangeCount);
        for (const auto &range : ranges) {
            if (range.contains(character)) {
                return true;
            }
        }
        return false;
    }
    }
    return false;
}

}
