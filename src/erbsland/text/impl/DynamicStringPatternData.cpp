// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "DynamicStringPatternData.hpp"

#include "ThrowHelper.hpp"

#include "../CharSet.hpp"
#include "../u32/U32StringEditor.hpp"

#include "../../err/ParseError.hpp"

#include <limits>

namespace erbsland::text::impl {

auto DynamicStringPatternData::parse(
    StringCharReader &reader, const StringPatternParseMode mode, const bool trimTrailingWhitespace)
    -> StringPatternDataPtr {
    auto data = std::make_shared<DynamicStringPatternData>();
    try {
        data->parseElements(reader, mode, trimTrailingWhitespace);
        data->validate();
    } catch (const err::ParseError &error) {
        if (mode != StringPatternParseMode::PartialPathGlob) {
            throw;
        }
        throw err::ParseError{error.reason(), reader.position()};
    }
    return data;
}

void DynamicStringPatternData::parseElements(
    StringCharReader &reader, const StringPatternParseMode mode, const bool trimTrailingWhitespace) {
    const auto pathMode = mode == StringPatternParseMode::PartialPathGlob;
    if (mode != StringPatternParseMode::Divider) {
        _kind = StringPatternKind::Glob;
    }
    auto trailingWhitespace = std::size_t{};
    while (!reader.isAtEnd() && !(pathMode && isPathBoundary(reader.peek()))) {
        const auto character = reader.read();
        // Count only bare whitespace: escapes and sets preserve their represented characters.
        if (character == U' ' || character == U'\t') {
            ++trailingWhitespace;
        } else {
            trailingWhitespace = 0U;
        }
        parseElement(reader, character, mode);
    }
    if (pathMode) {
        if (trimTrailingWhitespace && reader.peek() != U'/') {
            _elements.resize(_elements.size() - trailingWhitespace);
        }
        validateRecursiveStar();
    }
}

void DynamicStringPatternData::parseElement(
    StringCharReader &reader, const Char character, const StringPatternParseMode mode) {
    if (character == U'\\') {
        appendCharacter(readEscapedCharacter(reader, mode));
    } else if (character == U'?') {
        appendOneChar();
    } else if (character == U'*') {
        parseStar(reader, mode);
    } else if (character == U'[') {
        parseSet(reader, mode);
    } else if (character == U']') {
        throwParseError("String pattern contains an unexpected closing bracket");
    } else {
        appendCharacter(character);
    }
}

void DynamicStringPatternData::parseStar(StringCharReader &reader, const StringPatternParseMode mode) {
    if (mode == StringPatternParseMode::PartialPathGlob && reader.readIf(U'*')) {
        _elements.push_back(StringPatternElement{.kind = StringPatternElementKind::RecursiveStar});
    } else if (mode == StringPatternParseMode::Divider) {
        appendDivider();
    } else {
        appendStar();
    }
}

void DynamicStringPatternData::parseSet(StringCharReader &reader, const StringPatternParseMode mode) {
    auto builder = U32StringEditor{};
    while (!reader.isAtEnd()) {
        auto character = reader.read();
        if (character == U']') {
            if (builder.isEmpty()) {
                throwParseError("String pattern character set must not be empty");
            }
            appendSetPattern(builder);
            return;
        }
        if (character == U'\\') {
            character = readEscapedCharacter(reader, mode);
        }
        builder.append(character);
    }
    throwParseError("String pattern character set is missing a closing bracket");
}

auto DynamicStringPatternData::readEscapedCharacter(StringCharReader &reader, const StringPatternParseMode mode)
    -> Char {
    if (reader.isAtEnd()) {
        throwParseError("String pattern escape is missing a character");
    }
    const auto character = reader.read();
    const auto coreEscape =
        character == U'?' || character == U'*' || character == U'[' || character == U']' || character == U'\\';
    const auto pathEscape = mode == StringPatternParseMode::PartialPathGlob && (character == U';' || character == U'#');
    if (!coreEscape && !pathEscape) {
        throwParseError("String pattern contains an unsupported escape");
    }
    return character;
}

auto DynamicStringPatternData::isPathBoundary(const Char character) noexcept -> bool {
    return character == U'/' || character == U';' || character == U'\n' || character == U'\r';
}

void DynamicStringPatternData::validateRecursiveStar() const {
    if (_elements.size() == 1U) {
        return;
    }
    for (const auto &element : _elements) {
        if (element.kind == StringPatternElementKind::RecursiveStar) {
            throwParseError("A recursive wildcard must be a complete path component");
        }
    }
}

auto DynamicStringPatternData::view() const noexcept -> StringPatternView {
    return {.elements = std::span{_elements}, .ranges = std::span{_ranges}, .divider = _divider, .kind = _kind};
}

void DynamicStringPatternData::appendOneChar() {
    _elements.push_back(StringPatternElement{.kind = StringPatternElementKind::OneChar});
}

void DynamicStringPatternData::appendCharacter(const Char character) {
    if (!character.isValidUnicode()) {
        throwParseError("String pattern contains an invalid character");
    }
    _elements.push_back(
        StringPatternElement{
            .kind = StringPatternElementKind::Character, .character = character, .rangeOffset = 0U, .rangeCount = 0U});
}

void DynamicStringPatternData::appendSet(const CharSet &charSet) {
    if (charSet.isEmpty()) {
        throwParseError("String pattern character set must not be empty");
    }
    const auto rangeOffset = checkedRangeCount(_ranges.size());
    auto rangeCountValue = std::size_t{0};
    charSet.forEach([&rangeCountValue](const CharRange) noexcept -> void { ++rangeCountValue; });
    if (rangeCountValue > std::numeric_limits<std::uint16_t>::max() - _ranges.size()) {
        throwParseError("String pattern has too many ranges");
    }
    const auto rangeCount = checkedRangeCount(rangeCountValue);
    _elements.push_back(
        StringPatternElement{
            .kind = StringPatternElementKind::Set,
            .character = {},
            .rangeOffset = rangeOffset,
            .rangeCount = rangeCount});
    charSet.forEach([this](const CharRange range) -> void { _ranges.push_back(range); });
}

void DynamicStringPatternData::appendSetPattern(const U32String &pattern) {
    appendSet(CharSet::fromPattern(pattern));
}

void DynamicStringPatternData::appendStar() {
    if (_elements.empty() || _elements.back().kind != StringPatternElementKind::AnyText) {
        _elements.push_back(StringPatternElement{.kind = StringPatternElementKind::AnyText});
    }
}

void DynamicStringPatternData::appendDivider() {
    if (_divider != cNoStringPatternDivider) {
        throwParseError("String pattern contains more than one asterisk");
    }
    _divider = _elements.size();
}

void DynamicStringPatternData::validate() const {
    if (_divider == 0U && _elements.empty()) {
        throwParseError("String pattern asterisk must not stand alone");
    }
}

auto DynamicStringPatternData::checkedRangeCount(const std::size_t count) const -> std::uint16_t {
    if (count > std::numeric_limits<std::uint16_t>::max()) {
        throwParseError("String pattern has too many ranges");
    }
    return static_cast<std::uint16_t>(count);
}

}
