// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "KeyDecoder.hpp"

#include "../../text/impl/UnicodeData.hpp"
#include "../../text/u8/impl/U8Encoding.hpp"

#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace erbsland::cterm::impl {

namespace {

[[nodiscard]] auto byteIndex(const std::size_t value) noexcept -> unit::ByteIndex {
    return unit::ByteIndex::fromSizeT(value);
}

}

auto KeyDecoder::simpleSequenceDefinitions() noexcept -> const std::array<SimpleSequenceDefinition, 6> & {
    static constexpr auto cSimpleSequenceDefinitions = std::array<SimpleSequenceDefinition, 6>{{
        {"\n", Key::Enter},
        {"\r", Key::Enter},
        {"\t", Key::Tab},
        {" ", Key::Space},
        {"\x08", Key::Backspace},
        {"\x7f", Key::Backspace},
    }};
    return cSimpleSequenceDefinitions;
}

auto KeyDecoder::createCharacterKey(const CombinedBlock &character) noexcept -> Key {
    if (character.codePointCount() <= 1) {
        return Key{Key::Character, character.mainCodePoint()};
    }
    return Key{Key::Combined, character.utf32()};
}

auto KeyDecoder::parseUtf8CodePointPrefix(const unit::ByteIndex offset) const noexcept -> CharParseResult {
    const auto startOffset = offset.toSizeT();
    if (startOffset >= _text.size()) {
        return CharParseResult{KeyParseStatus::Invalid, offset};
    }
    const auto firstByte = static_cast<uint8_t>(_text[startOffset]);
    if ((firstByte & 0b11000000U) == 0b10000000U) {
        return CharParseResult{KeyParseStatus::Invalid, byteIndex(startOffset + 1U)};
    }
    auto requiredByteCount = std::size_t{1U};
    if (firstByte < 0x80U) {
        requiredByteCount = 1U;
    } else if ((firstByte & 0b11100000U) == 0b11000000U && firstByte >= 0b11000010U) {
        requiredByteCount = 2U;
    } else if ((firstByte & 0b11110000U) == 0b11100000U) {
        requiredByteCount = 3U;
    } else if ((firstByte & 0b11111000U) == 0b11110000U && firstByte < 0b11110101U) {
        requiredByteCount = 4U;
    } else {
        return CharParseResult{KeyParseStatus::Invalid, byteIndex(startOffset + 1U)};
    }
    if (_text.size() - startOffset < requiredByteCount) {
        return CharParseResult{KeyParseStatus::NeedMoreData, offset};
    }
    auto position = offset;
    try {
        const auto character =
            text::impl::utf8::decodeCharOrThrow(std::span<const char>{_text.data(), _text.size()}, position);
        return CharParseResult{KeyParseStatus::Parsed, character, position};
    } catch (...) {
        return CharParseResult{KeyParseStatus::Invalid, byteIndex(startOffset + 1U)};
    }
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

auto KeyDecoder::parseCsiParameters(const std::string_view text) noexcept -> std::optional<std::vector<int>> {
    auto result = std::vector<int>{};
    if (text.empty()) {
        return result;
    }
    auto value = 0;
    auto digitCount = 0;
    auto hasDigit = false;
    constexpr static auto cMaximumDigitCount = 8;
    constexpr static auto cMaximumParameterCount = 16;
    for (const auto character : text) {
        if (character >= '0' && character <= '9') {
            if (digitCount >= cMaximumDigitCount) {
                return std::nullopt;
            }
            hasDigit = true;
            value = value * 10 + (character - '0');
            digitCount++;
            continue;
        }
        if (character == ';') {
            if (result.size() >= cMaximumParameterCount) {
                return std::nullopt;
            }
            if (!hasDigit) {
                result.emplace_back(1);
            } else {
                result.emplace_back(value);
            }
            value = 0;
            hasDigit = false;
            digitCount = 0;
            continue;
        }
        return std::nullopt;
    }
    result.emplace_back(hasDigit ? value : 1);
    return result;
}

auto KeyDecoder::findCsiFinalByte(const std::string_view text) noexcept -> std::optional<std::size_t> {
    for (auto index = std::size_t{2}; index < text.size(); ++index) {
        const auto byte = static_cast<unsigned char>(text[index]);
        if (byte >= 0x40U && byte <= 0x7eU) {
            return index;
        }
    }
    return std::nullopt;
}

auto KeyDecoder::keyFromCsiFinal(const char finalByte) noexcept -> Key::Type {
    switch (finalByte) {
    case 'A':
        return Key::Up;
    case 'B':
        return Key::Down;
    case 'C':
        return Key::Right;
    case 'D':
        return Key::Left;
    case 'H':
        return Key::Home;
    case 'F':
        return Key::End;
    case 'P':
        return Key::F1;
    case 'Q':
        return Key::F2;
    case 'R':
        return Key::F3;
    case 'S':
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

auto KeyDecoder::decodeCsi(const std::string_view text) noexcept -> ParseResult {
    const auto finalIndex = findCsiFinalByte(text);
    if (!finalIndex.has_value()) {
        return ParseResult{KeyParseStatus::NeedMoreData, byteIndex(0)};
    }
    const auto sequenceSize = *finalIndex + 1;
    const auto finalByte = text[*finalIndex];
    if (finalByte == 'Z') {
        if (*finalIndex == 2) {
            return ParseResult{KeyParseStatus::Parsed, Key{Key::BackTab}, byteIndex(sequenceSize)};
        }
        return ParseResult{KeyParseStatus::Invalid, byteIndex(sequenceSize)};
    }

    const auto parameters = parseCsiParameters(text.substr(2, *finalIndex - 2));
    if (!parameters.has_value()) {
        return ParseResult{KeyParseStatus::Invalid, byteIndex(sequenceSize)};
    }

    auto type = Key::None;
    auto modifiers = KeyModifiers{};
    if (finalByte == '~') {
        if (parameters->empty()) {
            return ParseResult{KeyParseStatus::Invalid, byteIndex(sequenceSize)};
        }
        type = keyFromCsiTildeParameter((*parameters)[0]);
        if (parameters->size() >= 2) {
            const auto parsedModifiers = parseModifierParameter((*parameters)[1]);
            if (!parsedModifiers.has_value()) {
                return ParseResult{KeyParseStatus::Invalid, byteIndex(sequenceSize)};
            }
            modifiers = *parsedModifiers;
        }
    } else {
        type = keyFromCsiFinal(finalByte);
        if (parameters->size() >= 2) {
            const auto parsedModifiers = parseModifierParameter((*parameters)[1]);
            if (!parsedModifiers.has_value()) {
                return ParseResult{KeyParseStatus::Invalid, byteIndex(sequenceSize)};
            }
            modifiers = *parsedModifiers;
        } else if (parameters->size() == 1 && (*parameters)[0] != 1) {
            const auto parsedModifiers = parseModifierParameter((*parameters)[0]);
            if (!parsedModifiers.has_value()) {
                return ParseResult{KeyParseStatus::Invalid, byteIndex(sequenceSize)};
            }
            modifiers = *parsedModifiers;
        }
    }

    if (type == Key::None || parameters->size() > 2) {
        return ParseResult{KeyParseStatus::Invalid, byteIndex(sequenceSize)};
    }
    return ParseResult{KeyParseStatus::Parsed, Key{type, modifiers}, byteIndex(sequenceSize)};
}

auto KeyDecoder::decodeSs3(const std::string_view text) noexcept -> ParseResult {
    if (text.size() < 3) {
        return ParseResult{KeyParseStatus::NeedMoreData, byteIndex(0)};
    }
    switch (text[2]) {
    case 'H':
        return ParseResult{KeyParseStatus::Parsed, Key{Key::Home}, byteIndex(3)};
    case 'F':
        return ParseResult{KeyParseStatus::Parsed, Key{Key::End}, byteIndex(3)};
    case 'P':
        return ParseResult{KeyParseStatus::Parsed, Key{Key::F1}, byteIndex(3)};
    case 'Q':
        return ParseResult{KeyParseStatus::Parsed, Key{Key::F2}, byteIndex(3)};
    case 'R':
        return ParseResult{KeyParseStatus::Parsed, Key{Key::F3}, byteIndex(3)};
    case 'S':
        return ParseResult{KeyParseStatus::Parsed, Key{Key::F4}, byteIndex(3)};
    default:
        return ParseResult{KeyParseStatus::Invalid, byteIndex(3)};
    }
}

auto KeyDecoder::parseConsoleInputPrefix() const noexcept -> ParseResult {
    if (_text.empty()) {
        return ParseResult{KeyParseStatus::Invalid, byteIndex(0)};
    }

    for (const auto &definition : simpleSequenceDefinitions()) {
        if (_text.starts_with(definition.sequence)) {
            return ParseResult{KeyParseStatus::Parsed, Key{definition.type}, byteIndex(definition.sequence.size())};
        }
    }

    if (_text[0] == '\x1b') {
        if (_text.size() == 1) {
            return ParseResult{KeyParseStatus::NeedMoreData, byteIndex(0)};
        }
        if (_text[1] == '[') {
            return decodeCsi(_text);
        }
        if (_text[1] == 'O') {
            return decodeSs3(_text);
        }
        return ParseResult{KeyParseStatus::Invalid, byteIndex(2)};
    }

    const auto firstCodePoint = parseUtf8CodePointPrefix(byteIndex(0));
    if (firstCodePoint.status() != KeyParseStatus::Parsed) {
        return ParseResult{firstCodePoint.status(), firstCodePoint.consumedByteCount()};
    }
    const auto baseCodePoint = firstCodePoint.character();
    if (CombinedBlock::isControlCode(baseCodePoint)) {
        return ParseResult{KeyParseStatus::Invalid, firstCodePoint.consumedByteCount()};
    }
    if (text::impl::unicodeDisplayWidthFor(baseCodePoint.toRawValue()) == 0) {
        return ParseResult{KeyParseStatus::Invalid, firstCodePoint.consumedByteCount()};
    }

    auto character = CombinedBlock{baseCodePoint};
    auto offset = firstCodePoint.consumedByteCount();
    while (offset.toSizeT() < _text.size()) {
        const auto nextCodePoint = parseUtf8CodePointPrefix(offset);
        if (nextCodePoint.status() == KeyParseStatus::NeedMoreData) {
            return ParseResult{KeyParseStatus::NeedMoreData, byteIndex(0)};
        }
        if (nextCodePoint.status() != KeyParseStatus::Parsed) {
            return ParseResult{KeyParseStatus::Parsed, createCharacterKey(character), offset};
        }
        const auto codePoint = nextCodePoint.character();
        if (CombinedBlock::isControlCode(codePoint)) {
            return ParseResult{KeyParseStatus::Parsed, createCharacterKey(character), offset};
        }
        if (text::impl::unicodeDisplayWidthFor(codePoint.toRawValue()) != 0) {
            return ParseResult{KeyParseStatus::Parsed, createCharacterKey(character), offset};
        }
        character = character.withCombining(codePoint);
        offset = nextCodePoint.consumedByteCount();
    }
    return ParseResult{KeyParseStatus::Parsed, createCharacterKey(character), offset};
}

auto KeyDecoder::decodeConsoleInput() const noexcept -> Key {
    if (_text == "\x1b") {
        return Key{Key::Escape};
    }
    const auto result = parseConsoleInputPrefix();
    if (result.status() == KeyParseStatus::Parsed && result.consumedByteCount().toSizeT() == _text.size()) {
        return result.key();
    }
    return {};
}

}
