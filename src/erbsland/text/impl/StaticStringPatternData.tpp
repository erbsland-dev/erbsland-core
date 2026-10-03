// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

namespace erbsland::text::impl {

template <std::size_t tMaxElements, std::size_t tMaxRanges>
template <pattern::AnyElement... Args>
StaticStringPatternData<tMaxElements, tMaxRanges>::StaticStringPatternData(
    const StringPatternKind kind, const Args &...elements) :
    _kind{kind} {
    (append(elements), ...);
    validate();
}

template <std::size_t tMaxElements, std::size_t tMaxRanges>
auto StaticStringPatternData<tMaxElements, tMaxRanges>::view() const noexcept -> StringPatternView {
    return {
        .elements = std::span{_elements}.first(_elementCount),
        .ranges = std::span{_ranges}.first(_rangeCount),
        .divider = _divider,
        .kind = _kind};
}

template <std::size_t tMaxElements, std::size_t tMaxRanges>
void StaticStringPatternData<tMaxElements, tMaxRanges>::append(const pattern::Text &text) {
    for (const auto codePoint : text.view()) {
        const auto character = Char{codePoint};
        appendCharacter(character.isValidUnicode() ? character : Char::replacement());
    }
}

template <std::size_t tMaxElements, std::size_t tMaxRanges>
void StaticStringPatternData<tMaxElements, tMaxRanges>::append(const pattern::OneChar &) {
    requirePattern(_elementCount < tMaxElements, StringLiteral{"String pattern has too many elements"});
    _elements[_elementCount] = StringPatternElement{.kind = StringPatternElementKind::OneChar};
    ++_elementCount;
}

template <std::size_t tMaxElements, std::size_t tMaxRanges>
void StaticStringPatternData<tMaxElements, tMaxRanges>::append(const pattern::Range &range) {
    appendRange(range.charRange());
}

template <std::size_t tMaxElements, std::size_t tMaxRanges>
void StaticStringPatternData<tMaxElements, tMaxRanges>::append(const pattern::Set &set) {
    requirePattern(set.count() > 0U, StringLiteral{"String pattern set must not be empty"});
    requirePattern(_elementCount < tMaxElements, StringLiteral{"String pattern has too many elements"});
    requirePattern(_rangeCount + set.count() <= tMaxRanges, StringLiteral{"String pattern has too many ranges"});
    const auto rangeOffset = checkedRangeCount(_rangeCount);
    const auto rangeCount = checkedRangeCount(set.count());
    _elements[_elementCount] = StringPatternElement{
        .kind = StringPatternElementKind::Set, .character = {}, .rangeOffset = rangeOffset, .rangeCount = rangeCount};
    for (auto i = std::size_t{0U}; i < set.count(); ++i) {
        _ranges[_rangeCount] = set.range(i);
        ++_rangeCount;
    }
    ++_elementCount;
}

template <std::size_t tMaxElements, std::size_t tMaxRanges>
void StaticStringPatternData<tMaxElements, tMaxRanges>::append(const pattern::Divider &) {
    requirePattern(_kind == StringPatternKind::Divider, StringLiteral{"Glob patterns do not accept Divider elements"});
    appendDivider();
}

template <std::size_t tMaxElements, std::size_t tMaxRanges>
void StaticStringPatternData<tMaxElements, tMaxRanges>::append(const pattern::Star &) {
    requirePattern(_kind == StringPatternKind::Glob, StringLiteral{"Divider patterns do not accept Star elements"});
    if (_elementCount > 0U && _elements[_elementCount - 1U].kind == StringPatternElementKind::AnyText) {
        return;
    }
    requirePattern(_elementCount < tMaxElements, StringLiteral{"String pattern has too many elements"});
    _elements[_elementCount] = StringPatternElement{.kind = StringPatternElementKind::AnyText};
    ++_elementCount;
}

template <std::size_t tMaxElements, std::size_t tMaxRanges>
void StaticStringPatternData<tMaxElements, tMaxRanges>::appendCharacter(const Char character) {
    requirePattern(character.isValidUnicode(), StringLiteral{"String pattern contains an invalid character"});
    requirePattern(_elementCount < tMaxElements, StringLiteral{"String pattern has too many elements"});
    _elements[_elementCount] = StringPatternElement{
        .kind = StringPatternElementKind::Character, .character = character, .rangeOffset = 0U, .rangeCount = 0U};
    ++_elementCount;
}

template <std::size_t tMaxElements, std::size_t tMaxRanges>
void StaticStringPatternData<tMaxElements, tMaxRanges>::appendRange(const CharRange range) {
    requirePattern(!range.isEmpty(), StringLiteral{"String pattern range must not be empty"});
    requirePattern(_elementCount < tMaxElements, StringLiteral{"String pattern has too many elements"});
    requirePattern(_rangeCount < tMaxRanges, StringLiteral{"String pattern has too many ranges"});
    _elements[_elementCount] = StringPatternElement{
        .kind = StringPatternElementKind::Set,
        .character = {},
        .rangeOffset = checkedRangeCount(_rangeCount),
        .rangeCount = 1U};
    _ranges[_rangeCount] = range;
    ++_rangeCount;
    ++_elementCount;
}

template <std::size_t tMaxElements, std::size_t tMaxRanges>
void StaticStringPatternData<tMaxElements, tMaxRanges>::appendDivider() {
    requirePattern(
        _divider == cNoStringPatternDivider, StringLiteral{"String pattern contains more than one asterisk"});
    _divider = _elementCount;
}

template <std::size_t tMaxElements, std::size_t tMaxRanges>
void StaticStringPatternData<tMaxElements, tMaxRanges>::validate() const {
    requirePattern(_divider != 0U || _elementCount > 0U, StringLiteral{"String pattern asterisk must not stand alone"});
}

template <std::size_t tMaxElements, std::size_t tMaxRanges>
void StaticStringPatternData<tMaxElements, tMaxRanges>::requirePattern(
    const bool condition, const StringLiteral &reason) const {
    if (!condition) {
        throw err::ParameterFailure{reason, StringLiteral{"StringPattern"}};
    }
}

template <std::size_t tMaxElements, std::size_t tMaxRanges>
auto StaticStringPatternData<tMaxElements, tMaxRanges>::checkedRangeCount(const std::size_t count) const
    -> std::uint16_t {
    requirePattern(
        count <= std::numeric_limits<std::uint16_t>::max(), StringLiteral{"String pattern has too many ranges"});
    return static_cast<std::uint16_t>(count);
}

}
