// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Parser.hpp"

#include "../AnyString.hpp"

#include "../../err/ParseError.hpp"

namespace erbsland::text::named_key {

Parser::Parser(StringCharReader &reader, const Format &format) : _reader{reader}, _format{format} {
    _format.validate();
}

void Parser::setAllowedKeys(util::Set<int> keys) {
    _allowedKeys = std::move(keys);
}

auto Parser::readEntry() -> Entry {
    if (_ended) {
        return {};
    }
    if (!_firstEntry) {
        if (isAtTerminator()) {
            return readEnd();
        }
        if (!_reader.advanceIf(_format.listSeparator())) {
            throwError("Named-key entries must be separated by the configured list separator", _reader.position());
        }
        if (isAtTerminator() || _reader.peek() == _format.listSeparator()) {
            throwError("A named-key entry must not be empty", _reader.position());
        }
    } else if (isAtTerminator()) {
        return readEnd();
    } else if (_reader.peek() == _format.listSeparator()) {
        throwError("A named-key entry must not be empty", _reader.position());
    }

    const auto position = _reader.position();
    const auto raw = captureRawEntry();
    _firstEntry = false;
    return parseRawEntry(raw, position);
}

auto Parser::readAllEntries() -> util::List<Entry> {
    auto result = util::List<Entry>{};
    while (true) {
        auto entry = readEntry();
        if (entry.isEnd()) {
            return result;
        }
        result.append(std::move(entry));
    }
}

auto Parser::isAtTerminator() const noexcept -> bool {
    if (_format.stopCharacter().isEndOfData()) {
        return _reader.isAtEnd();
    }
    return _reader.peek() == _format.stopCharacter() || _reader.isAtEnd();
}

auto Parser::readEnd() -> Entry {
    if (!_format.stopCharacter().isEndOfData()) {
        if (_reader.isAtEnd()) {
            throwError("Named-key list is missing its stop character", _reader.position());
        }
        _reader.advance();
    }
    _ended = true;
    return {};
}

auto Parser::captureRawEntry() -> String {
    _reader.startCapture();
    while (!_reader.isAtEnd()) {
        const auto character = _reader.peek();
        if (character == _format.listSeparator() ||
            (!_format.stopCharacter().isEndOfData() && character == _format.stopCharacter())) {
            break;
        }
        const auto consumed = _reader.read();
        if (consumed.isSignal() || !consumed.isSafeUnicode()) {
            throwError("Named-key entry contains an invalid character", _reader.position());
        }
    }
    const auto result = _reader.takeCapture().toString();
    if (result.isEmpty()) {
        throwError("A named-key entry must not be empty", _reader.position());
    }
    return result;
}

auto Parser::parseRawEntry(const String &raw, const unit::CpIndex position) -> Entry {
    if (_mode == Mode::Values) {
        return parsePositionalValue(raw, position);
    }

    auto reader = StringCharReader{raw};
    auto prefix = Char::noCodePoint();
    if (_format.allowedKeyPrefixes().contains(reader.peek())) {
        prefix = reader.read();
    }

    reader.startCapture();
    while (
        !reader.isAtEnd() && reader.peek() != _format.valueSeparator() &&
        !_format.valueWithoutKeySeparatorChars().contains(reader.peek())) {
        reader.advance();
    }
    const auto keyText = reader.takeCapture().toString();
    const auto hasExplicitValue = reader.peek() == _format.valueSeparator();
    const auto keyIndex = keyText.isEmpty() ? std::optional<int>{} : _format.keyIndex(keyText);

    if (keyIndex.has_value()) {
        if (_allowedKeys.has_value() && !_allowedKeys->contains(keyIndex.value())) {
            throwError("Named-key entry uses a key that is not allowed in this context", position);
        }
        _mode = Mode::Keys;
        markKeySeen(keyIndex.value(), position);
        return parseKeyEntry(reader, prefix, keyIndex.value(), position);
    }

    if (hasExplicitValue) {
        throwError("Named-key entry contains an unknown key", position);
    }
    if (_mode == Mode::Keys || !_format.valueListAllowed()) {
        throwError("Named-key entry contains an unknown key", position);
    }
    return parsePositionalValue(raw, position);
}

auto Parser::parseKeyEntry(
    StringCharReader &reader, const Char prefix, const int keyIndex, const unit::CpIndex position) -> Entry {
    if (reader.isAtEnd()) {
        if (!_format.keysWithoutValuesAllowed()) {
            throwError("Named-key entry is missing a value", position);
        }
        return {EntryKind::Key, prefix, keyIndex, {}};
    }
    if (!_format.valuesAllowed()) {
        throwError("Named-key values are not allowed in this format", position);
    }
    if (reader.peek() == _format.valueSeparator()) {
        reader.advance();
    }
    reader.startCapture();
    while (!reader.isAtEnd()) {
        reader.advance();
    }
    auto value = reader.takeCapture().toString();
    if (value.isEmpty()) {
        throwError("Named-key entry is missing a value", position);
    }
    validateValue(value, position);
    return {EntryKind::KeyWithValue, prefix, keyIndex, std::move(value)};
}

auto Parser::parsePositionalValue(const String &raw, const unit::CpIndex position) -> Entry {
    _mode = Mode::Values;
    ++_valueCount;
    if (_valueCount > _format.maximumValues()) {
        throwError("Named-key value list contains too many values", position);
    }
    validateValue(raw, position);
    return {EntryKind::Value, Char::noCodePoint(), -1, raw};
}

void Parser::validateValue(const String &value, const unit::CpIndex position) const {
    if (value.characterLength() > _format.maximumValueLength()) {
        throwError("Named-key value exceeds the configured length limit", position);
    }
    if (_format.allowedValueChars().isEmpty()) {
        return;
    }
    auto reader = StringCharReader{value};
    while (!reader.isAtEnd()) {
        if (!_format.allowedValueChars().contains(reader.read())) {
            throwError("Named-key value contains a disallowed character", position);
        }
    }
}

void Parser::markKeySeen(const int keyIndex, const unit::CpIndex position) {
    if (!_format.uniqueKeysRequired()) {
        return;
    }
    if (_seenKeys.contains(keyIndex)) {
        throwError("Named-key entry is specified more than once", position);
    }
    _seenKeys.insert(keyIndex);
}

void Parser::throwError(const std::string_view reason, const unit::CpIndex position) {
    throw err::ParseError{reason, position};
}

}
