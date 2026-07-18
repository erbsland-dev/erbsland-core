// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StringPatternData.hpp"

namespace erbsland::text::impl {

using namespace unit;

auto StringPatternData::matches(const U8String &text) const noexcept -> bool {
    return match(view(), text).matched;
}

auto StringPatternData::matches(const U16String &text) const noexcept -> bool {
    return match(view(), text).matched;
}

auto StringPatternData::matches(const U32String &text) const noexcept -> bool {
    return match(view(), text).matched;
}

auto StringPatternData::trim(U8String &text) const noexcept -> bool {
    const auto trimmedView = trimmed(text);
    if (trimmedView == text) {
        return false;
    }
    text = trimmedView;
    return true;
}

auto StringPatternData::trim(U16String &text) const noexcept -> bool {
    const auto trimmedView = trimmed(text);
    if (trimmedView == text) {
        return false;
    }
    text = trimmedView;
    return true;
}

auto StringPatternData::trim(U32String &text) const noexcept -> bool {
    const auto trimmedView = trimmed(text);
    if (trimmedView == text) {
        return false;
    }
    text = trimmedView;
    return true;
}

auto StringPatternData::trim(U8StringEditor &text) const -> bool {
    const auto textView = U8String{text};
    const auto trimmedView = trimmed(textView);
    if (trimmedView == textView) {
        return false;
    }
    text = U8StringEditor{trimmedView};
    return true;
}

auto StringPatternData::trim(U16StringEditor &text) const -> bool {
    const auto textView = U16String{text};
    const auto trimmedView = trimmed(textView);
    if (trimmedView == textView) {
        return false;
    }
    text = U16StringEditor{trimmedView};
    return true;
}

auto StringPatternData::trim(U32StringEditor &text) const -> bool {
    const auto textView = U32String{text};
    const auto trimmedView = trimmed(textView);
    if (trimmedView == textView) {
        return false;
    }
    text = U32StringEditor{trimmedView};
    return true;
}

auto StringPatternData::trimmed(const U8String &text) const noexcept -> U8String {
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
    return text.slice(ByteRange{result.frontEnd, result.suffixStart});
}

auto StringPatternData::trimmed(const U16String &text) const noexcept -> U16String {
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
    return text.slice(U16DataRange{result.frontEnd, result.suffixStart});
}

auto StringPatternData::trimmed(const U32String &text) const noexcept -> U32String {
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
    return text.slice(CpRange{result.frontEnd, result.suffixStart});
}

auto StringPatternData::split(const U8String &text) const noexcept -> std::pair<U8String, U8String> {
    const auto patternView = view();
    const auto result = match(patternView, text);
    if (!result.matched) {
        return {U8String{}, text};
    }
    if (patternView.divider == 0U) {
        return text.splitAt(result.suffixStart);
    }
    auto [prefix, rest] = text.splitAt(result.frontEnd);
    if (patternView.divider != cNoStringPatternDivider && patternView.divider < patternView.elements.size()) {
        rest = text.slice(ByteRange{result.frontEnd, result.suffixStart});
    }
    return {prefix, rest};
}

auto StringPatternData::split(const U16String &text) const noexcept -> std::pair<U16String, U16String> {
    const auto patternView = view();
    const auto result = match(patternView, text);
    if (!result.matched) {
        return {U16String{}, text};
    }
    if (patternView.divider == 0U) {
        return text.splitAt(result.suffixStart);
    }
    auto [prefix, rest] = text.splitAt(result.frontEnd);
    if (patternView.divider != cNoStringPatternDivider && patternView.divider < patternView.elements.size()) {
        rest = text.slice(U16DataRange{result.frontEnd, result.suffixStart});
    }
    return {prefix, rest};
}

auto StringPatternData::split(const U32String &text) const noexcept -> std::pair<U32String, U32String> {
    const auto patternView = view();
    const auto result = match(patternView, text);
    if (!result.matched) {
        return {U32String{}, text};
    }
    if (patternView.divider == 0U) {
        return text.splitAt(result.suffixStart);
    }
    auto [prefix, rest] = text.splitAt(result.frontEnd);
    if (patternView.divider != cNoStringPatternDivider && patternView.divider < patternView.elements.size()) {
        rest = text.slice(CpRange{result.frontEnd, result.suffixStart});
    }
    return {prefix, rest};
}

auto StringPatternData::length(const U8String &text) const noexcept -> ByteLength {
    const auto patternView = view();
    const auto result = match(patternView, text);
    if (!result.matched) {
        return ByteLength::zero();
    }
    if (patternView.divider == 0U) {
        return lengthToEnd(text.length(), result.suffixStart);
    }
    return lengthFromStart<ByteLength>(result.frontEnd);
}

auto StringPatternData::length(const U16String &text) const noexcept -> U16DataLength {
    const auto patternView = view();
    const auto result = match(patternView, text);
    if (!result.matched) {
        return U16DataLength::zero();
    }
    if (patternView.divider == 0U) {
        return lengthToEnd(text.length(), result.suffixStart);
    }
    return lengthFromStart<U16DataLength>(result.frontEnd);
}

auto StringPatternData::length(const U32String &text) const noexcept -> CpLength {
    const auto patternView = view();
    const auto result = match(patternView, text);
    if (!result.matched) {
        return CpLength::zero();
    }
    if (patternView.divider == 0U) {
        return lengthToEnd(text.length(), result.suffixStart);
    }
    return lengthFromStart<CpLength>(result.frontEnd);
}

auto StringPatternData::index(const U8String &text) const noexcept -> ByteIndex {
    const auto patternView = view();
    const auto result = match(patternView, text);
    if (!result.matched) {
        return ByteIndex::noIndex();
    }
    return patternView.divider == 0U ? result.suffixStart : result.frontEnd;
}

auto StringPatternData::index(const U16String &text) const noexcept -> U16DataIndex {
    const auto patternView = view();
    const auto result = match(patternView, text);
    if (!result.matched) {
        return U16DataIndex::noIndex();
    }
    return patternView.divider == 0U ? result.suffixStart : result.frontEnd;
}

auto StringPatternData::index(const U32String &text) const noexcept -> CpIndex {
    const auto patternView = view();
    const auto result = match(patternView, text);
    if (!result.matched) {
        return CpIndex::noIndex();
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
