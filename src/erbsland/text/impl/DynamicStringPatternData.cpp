// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "DynamicStringPatternData.hpp"

#include "ThrowHelper.hpp"

#include "../CharSet.hpp"
#include "../StringBuilder.hpp"
#include "../StringKind.hpp"
#include "../u32/U32String.hpp"

#include <limits>

namespace erbsland::text::impl {

auto DynamicStringPatternData::parse(StringCharReader &reader) -> StringPatternDataPtr {
    auto data = std::make_shared<DynamicStringPatternData>();
    while (!reader.isAtEnd()) {
        const auto character = reader.read();
        if (character == Char{U'\\'}) {
            if (reader.isAtEnd()) {
                text::impl::throwParseError("String pattern escape is missing a character");
            }
            data->appendEscapedCharacter(reader.read());
            continue;
        }
        if (character == Char{U'?'}) {
            data->appendOneChar();
            continue;
        }
        if (character == Char{U'*'}) {
            data->appendDivider();
            continue;
        }
        if (character == Char{U'['}) {
            auto builder = StringBuilder{StringKind::U32};
            auto hasContent = false;
            auto isClosed = false;
            while (!reader.isAtEnd()) {
                const auto setCharacter = reader.read();
                if (setCharacter == Char{U']'}) {
                    if (!hasContent) {
                        text::impl::throwParseError("String pattern character set must not be empty");
                    }
                    data->appendSetPattern(builder.takeU32String());
                    hasContent = false;
                    isClosed = true;
                    break;
                }
                if (setCharacter == Char{U'\\'}) {
                    if (reader.isAtEnd()) {
                        text::impl::throwParseError("String pattern escape is missing a character");
                    }
                    const auto escapedCharacter = reader.read();
                    if (escapedCharacter != Char{U'?'} && escapedCharacter != Char{U'*'} &&
                        escapedCharacter != Char{U'['} && escapedCharacter != Char{U']'} &&
                        escapedCharacter != Char{U'\\'}) {
                        text::impl::throwParseError("String pattern contains an unsupported escape");
                    }
                    builder.append(escapedCharacter);
                    hasContent = true;
                    continue;
                }
                builder.append(setCharacter);
                hasContent = true;
            }
            if (!isClosed) {
                text::impl::throwParseError("String pattern character set is missing a closing bracket");
            }
            continue;
        }
        if (character == Char{U']'}) {
            text::impl::throwParseError("String pattern contains an unexpected closing bracket");
        }
        data->appendCharacter(character);
    }
    data->validate();
    return data;
}

auto DynamicStringPatternData::view() const noexcept -> StringPatternView {
    return {.elements = std::span{_elements}, .ranges = std::span{_ranges}, .divider = _divider};
}

void DynamicStringPatternData::appendOneChar() {
    _elements.push_back(StringPatternElement{.kind = StringPatternElementKind::OneChar});
}

void DynamicStringPatternData::appendCharacter(const Char character) {
    if (!character.isValidUnicode()) {
        text::impl::throwParseError("String pattern contains an invalid character");
    }
    _elements.push_back(
        StringPatternElement{
            .kind = StringPatternElementKind::Character, .character = character, .rangeOffset = 0U, .rangeCount = 0U});
}

void DynamicStringPatternData::appendSet(const CharSet &charSet) {
    if (charSet.isEmpty()) {
        text::impl::throwParseError("String pattern character set must not be empty");
    }
    if (_ranges.size() + charSet.ranges().size() > std::numeric_limits<std::uint16_t>::max()) {
        text::impl::throwParseError("String pattern has too many ranges");
    }
    const auto rangeOffset = checkedRangeCount(_ranges.size());
    const auto rangeCount = checkedRangeCount(charSet.ranges().size());
    _elements.push_back(
        StringPatternElement{
            .kind = StringPatternElementKind::Set,
            .character = {},
            .rangeOffset = rangeOffset,
            .rangeCount = rangeCount});
    for (const auto &range : charSet.ranges()) {
        _ranges.push_back(range);
    }
}

void DynamicStringPatternData::appendSetPattern(const U32StringView &pattern) {
    appendSet(CharSet::fromPattern(pattern));
}

void DynamicStringPatternData::appendDivider() {
    if (_divider != cNoStringPatternDivider) {
        text::impl::throwParseError("String pattern contains more than one asterisk");
    }
    _divider = _elements.size();
}

void DynamicStringPatternData::appendEscapedCharacter(const Char character) {
    if (character != Char{U'?'} && character != Char{U'*'} && character != Char{U'['} && character != Char{U']'} &&
        character != Char{U'\\'}) {
        text::impl::throwParseError("String pattern contains an unsupported escape");
    }
    appendCharacter(character);
}

void DynamicStringPatternData::validate() const {
    if (_divider == 0U && _elements.empty()) {
        text::impl::throwParseError("String pattern asterisk must not stand alone");
    }
}

auto DynamicStringPatternData::checkedRangeCount(const std::size_t count) const -> std::uint16_t {
    if (count > std::numeric_limits<std::uint16_t>::max()) {
        text::impl::throwParseError("String pattern has too many ranges");
    }
    return static_cast<std::uint16_t>(count);
}

}
