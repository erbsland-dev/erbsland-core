// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "KeyDecoder.hpp"

#include "../../text/EncodingErrorMode.hpp"
#include "../../text/IntegerParseOptions.hpp"
#include "../../text/Literals.hpp"
#include "../../text/ReadNumberStatus.hpp"
#include "../../text/StringCharReader.hpp"
#include "../../text/StringDecodeBuffer.hpp"
#include "../../unit/CpLength.hpp"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace erbsland::cterm::impl {

using namespace text;
using namespace text::literals;
using namespace unit;

constexpr auto cCsiPrefixLength = "\x1b["_el.length();
constexpr auto cCsiPrefixIndex = ByteIndex::end(cCsiPrefixLength);

auto KeyDecoder::simpleSequenceDefinitions() noexcept -> const std::array<SimpleSequenceDefinition, 6> & {
    static const auto cSimpleSequenceDefinitions = std::array<SimpleSequenceDefinition, 6>{{
        {"\n"_el, Key::Enter},
        {"\r"_el, Key::Enter},
        {"\t"_el, Key::Tab},
        {" "_el, Key::Space},
        {"\x08"_el, Key::Backspace},
        {"\x7f"_el, Key::Backspace},
    }};
    return cSimpleSequenceDefinitions;
}

auto KeyDecoder::createCharacterKey(const CombinedChar &character) noexcept -> Key {
    if (character.characterCount() <= CpLength::one()) {
        return Key{Key::Character, character.first()};
    }
    return Key{Key::Combined, character.toU32String()};
}

auto KeyDecoder::decodeCodePointPrefix(const String &text, const ByteIndex offset) noexcept -> CharParseResult {
    if (offset >= ByteIndex::end(text.length())) {
        return CharParseResult{KeyParseStatus::Invalid, offset};
    }

    const auto remainingLength = ByteLength::fromSizeT(text.length().toSizeT() - offset.toSizeT());
    const auto prefixLength = std::min(remainingLength, ByteLength{4U});
    auto buffer =
        StringDecodeBuffer{ByteLength{4U}, StringEncoding::Utf8, StringBomMode::Reject, EncodingErrorMode::Throw};

    for (auto byteOffset = std::size_t{0U}; byteOffset < prefixLength.toSizeT(); ++byteOffset) {
        const auto currentOffset = offset + ByteLength::fromSizeT(byteOffset);
        buffer.writeStringBytes(text.slice(ByteRange{currentOffset, ByteLength::one()}));

        const auto status = buffer.codePointStatus();
        if (status == StringDecodeBuffer::CodePointStatus::Invalid) {
            return CharParseResult{KeyParseStatus::Invalid, offset + ByteLength::one()};
        }
        if (buffer.decodableCharacters(CpLength::one()) == CpLength::one()) {
            const auto beforeLength = buffer.byteLength();
            try {
                const auto decodedText = buffer.takeString(CpLength::one());
                auto reader = StringCharReader{decodedText};
                const auto character = reader.read();
                const auto consumedLength = beforeLength - buffer.byteLength();
                return CharParseResult{KeyParseStatus::Parsed, character, offset + consumedLength};
            } catch (...) {
                return CharParseResult{KeyParseStatus::Invalid, offset + ByteLength::one()};
            }
        }
    }
    return CharParseResult{KeyParseStatus::NeedMoreData, offset};
}

auto KeyDecoder::parseModifierParameter(const int value) noexcept -> std::optional<KeyModifiers> {
    auto modifiers = KeyModifiers{};
    switch (value) {
    case 1:
        return modifiers;
    case 2:
        modifiers.set(KeyModifier::Shift);
        return modifiers;
    case 3:
        modifiers.set(KeyModifier::Alt);
        return modifiers;
    case 4:
        modifiers.set(KeyModifier::Shift);
        modifiers.set(KeyModifier::Alt);
        return modifiers;
    case 5:
        modifiers.set(KeyModifier::Control);
        return modifiers;
    case 6:
        modifiers.set(KeyModifier::Shift);
        modifiers.set(KeyModifier::Control);
        return modifiers;
    case 7:
        modifiers.set(KeyModifier::Alt);
        modifiers.set(KeyModifier::Control);
        return modifiers;
    case 8:
        modifiers.set(KeyModifier::Shift);
        modifiers.set(KeyModifier::Alt);
        modifiers.set(KeyModifier::Control);
        return modifiers;
    default:
        return std::nullopt;
    }
}

auto KeyDecoder::parseCsiParameters(const String &text) noexcept -> std::optional<std::vector<int>> {
    auto result = std::vector<int>{};
    if (text.isEmpty()) {
        return result;
    }
    constexpr static auto cMaximumDigitCount = CpLength{8U};
    constexpr static auto cMaximumParameterCount = 16U;
    auto parseOptions = IntegerParseOptions::parserDefault();
    parseOptions.setFixedBase(IntegerBase::Decimal).setMaximumDigits(cMaximumDigitCount);
    auto reader = StringCharReader{text};
    const auto appendParameter = [&result](const int value) noexcept -> bool {
        if (result.size() >= cMaximumParameterCount) {
            return false;
        }
        result.emplace_back(value);
        return true;
    };
    while (true) {
        if (reader.isAtEnd()) {
            if (!appendParameter(1)) {
                return std::nullopt;
            }
            return result;
        }
        if (reader.peek() == U';') {
            if (!appendParameter(1)) {
                return std::nullopt;
            }
            reader.advance();
            continue;
        }
        const auto parameter = reader.parseInteger(parseOptions);
        if (parameter.status != ReadNumberStatus::Success) {
            return std::nullopt;
        }
        if (!appendParameter(static_cast<int>(parameter.value))) {
            return std::nullopt;
        }
        if (reader.isAtEnd()) {
            return result;
        }
        if (!reader.advanceIf(U';')) {
            return std::nullopt;
        }
    }
}

auto KeyDecoder::findCsiFinalByte(const String &text) noexcept -> std::optional<ByteIndex> {
    auto index = cCsiPrefixIndex;
    const auto end = ByteIndex::end(text.length());
    while (index < end) {
        const auto currentIndex = index;
        const auto character = text.readCharAndAdvance(index);
        if (character.isAscii() && character.toRawValue() >= 0x40U && character.toRawValue() <= 0x7eU) {
            return currentIndex;
        }
    }
    return std::nullopt;
}

auto KeyDecoder::keyFromCsiFinal(const Char finalByte) noexcept -> Key::Type {
    switch (finalByte.toRawValue()) {
    case U'A':
        return Key::Up;
    case U'B':
        return Key::Down;
    case U'C':
        return Key::Right;
    case U'D':
        return Key::Left;
    case U'H':
        return Key::Home;
    case U'F':
        return Key::End;
    case U'P':
        return Key::F1;
    case U'Q':
        return Key::F2;
    case U'R':
        return Key::F3;
    case U'S':
        return Key::F4;
    default:
        return Key::None;
    }
}

auto KeyDecoder::keyFromCsiTildeParameter(const int parameter) noexcept -> Key::Type {
    switch (parameter) {
    case 1:
    case 7:
        return Key::Home;
    case 2:
        return Key::Insert;
    case 3:
        return Key::Delete;
    case 4:
    case 8:
        return Key::End;
    case 5:
        return Key::PageUp;
    case 6:
        return Key::PageDown;
    case 15:
        return Key::F5;
    case 17:
        return Key::F6;
    case 18:
        return Key::F7;
    case 19:
        return Key::F8;
    case 20:
        return Key::F9;
    case 21:
        return Key::F10;
    case 23:
        return Key::F11;
    case 24:
        return Key::F12;
    default:
        return Key::None;
    }
}

auto KeyDecoder::decodeCsi(const String &text) noexcept -> ParseResult {
    const auto finalIndex = findCsiFinalByte(text);
    if (!finalIndex.has_value()) {
        return ParseResult{KeyParseStatus::NeedMoreData, ByteIndex::zero()};
    }
    const auto sequenceSize = *finalIndex + ByteLength::one();
    const auto finalByte = text.charAt(*finalIndex);
    if (finalByte == U'Z') {
        if (*finalIndex == cCsiPrefixIndex) {
            return ParseResult{KeyParseStatus::Parsed, Key{Key::BackTab}, sequenceSize};
        }
        return ParseResult{KeyParseStatus::Invalid, sequenceSize};
    }

    const auto parameters = parseCsiParameters(text.slice(ByteRange{cCsiPrefixIndex, *finalIndex}));
    if (!parameters.has_value()) {
        return ParseResult{KeyParseStatus::Invalid, sequenceSize};
    }

    auto type = Key::None;
    auto modifiers = KeyModifiers{};
    if (finalByte == U'~') {
        if (parameters->empty()) {
            return ParseResult{KeyParseStatus::Invalid, sequenceSize};
        }
        type = keyFromCsiTildeParameter((*parameters)[0]);
        if (parameters->size() >= 2) {
            const auto parsedModifiers = parseModifierParameter((*parameters)[1]);
            if (!parsedModifiers.has_value()) {
                return ParseResult{KeyParseStatus::Invalid, sequenceSize};
            }
            modifiers = *parsedModifiers;
        }
    } else {
        type = keyFromCsiFinal(finalByte);
        if (parameters->size() >= 2) {
            const auto parsedModifiers = parseModifierParameter((*parameters)[1]);
            if (!parsedModifiers.has_value()) {
                return ParseResult{KeyParseStatus::Invalid, sequenceSize};
            }
            modifiers = *parsedModifiers;
        } else if (parameters->size() == 1 && (*parameters)[0] != 1) {
            const auto parsedModifiers = parseModifierParameter((*parameters)[0]);
            if (!parsedModifiers.has_value()) {
                return ParseResult{KeyParseStatus::Invalid, sequenceSize};
            }
            modifiers = *parsedModifiers;
        }
    }

    if (type == Key::None || parameters->size() > 2) {
        return ParseResult{KeyParseStatus::Invalid, sequenceSize};
    }
    return ParseResult{KeyParseStatus::Parsed, Key{type, modifiers}, sequenceSize};
}

auto KeyDecoder::decodeSs3(const String &text) noexcept -> ParseResult {
    if (text.length() < ByteLength{3U}) {
        return ParseResult{KeyParseStatus::NeedMoreData, ByteIndex::zero()};
    }
    switch (text.charAt(ByteIndex{2U}).toRawValue()) {
    case U'H':
        return ParseResult{KeyParseStatus::Parsed, Key{Key::Home}, ByteIndex{3}};
    case U'F':
        return ParseResult{KeyParseStatus::Parsed, Key{Key::End}, ByteIndex{3}};
    case U'P':
        return ParseResult{KeyParseStatus::Parsed, Key{Key::F1}, ByteIndex{3}};
    case U'Q':
        return ParseResult{KeyParseStatus::Parsed, Key{Key::F2}, ByteIndex{3}};
    case U'R':
        return ParseResult{KeyParseStatus::Parsed, Key{Key::F3}, ByteIndex{3}};
    case U'S':
        return ParseResult{KeyParseStatus::Parsed, Key{Key::F4}, ByteIndex{3}};
    default:
        return ParseResult{KeyParseStatus::Invalid, ByteIndex{3}};
    }
}

auto KeyDecoder::parseConsoleInputPrefix() const noexcept -> ParseResult {
    if (_text.isEmpty()) {
        return ParseResult{KeyParseStatus::Invalid, ByteIndex::zero()};
    }

    for (const auto &definition : simpleSequenceDefinitions()) {
        if (_text.startsWith(definition.sequence)) {
            return ParseResult{
                KeyParseStatus::Parsed, Key{definition.type}, ByteIndex::end(definition.sequence.length())};
        }
    }

    if (_text.startsWith("\x1b"_el)) {
        if (_text.length() == ByteLength::one()) {
            return ParseResult{KeyParseStatus::NeedMoreData, ByteIndex::zero()};
        }
        if (_text.charAt(ByteIndex::one()) == U'[') {
            return decodeCsi(_text);
        }
        if (_text.charAt(ByteIndex::one()) == U'O') {
            return decodeSs3(_text);
        }
        return ParseResult{KeyParseStatus::Invalid, ByteIndex{2}};
    }

    const auto firstCodePoint = decodeCodePointPrefix(_text, ByteIndex::zero());
    if (firstCodePoint.status() != KeyParseStatus::Parsed) {
        return ParseResult{firstCodePoint.status(), firstCodePoint.consumedByteCount()};
    }
    const auto baseCodePoint = firstCodePoint.character();
    if (baseCodePoint.isControl()) {
        return ParseResult{KeyParseStatus::Invalid, firstCodePoint.consumedByteCount()};
    }
    if (baseCodePoint.displayWidth() == 0) {
        return ParseResult{KeyParseStatus::Invalid, firstCodePoint.consumedByteCount()};
    }

    auto character = CombinedChar{baseCodePoint};
    auto offset = firstCodePoint.consumedByteCount();
    const auto textEnd = ByteIndex::end(_text.length());
    while (offset < textEnd) {
        const auto nextCodePoint = decodeCodePointPrefix(_text, offset);
        if (nextCodePoint.status() == KeyParseStatus::NeedMoreData) {
            return ParseResult{KeyParseStatus::NeedMoreData, ByteIndex::zero()};
        }
        if (nextCodePoint.status() != KeyParseStatus::Parsed) {
            return ParseResult{KeyParseStatus::Parsed, createCharacterKey(character), offset};
        }
        const auto codePoint = nextCodePoint.character();
        if (codePoint.isControl()) {
            return ParseResult{KeyParseStatus::Parsed, createCharacterKey(character), offset};
        }
        if (codePoint.displayWidth() != 0) {
            return ParseResult{KeyParseStatus::Parsed, createCharacterKey(character), offset};
        }
        character = character.withCombining(codePoint);
        offset = nextCodePoint.consumedByteCount();
    }
    return ParseResult{KeyParseStatus::Parsed, createCharacterKey(character), offset};
}

auto KeyDecoder::decodeConsoleInput() const noexcept -> Key {
    if (_text == "\x1b"_el) {
        return Key{Key::Escape};
    }
    const auto result = parseConsoleInputPrefix();
    if (result.status() == KeyParseStatus::Parsed && result.consumedByteCount() == ByteIndex::end(_text.length())) {
        return result.key();
    }
    return {};
}

}
