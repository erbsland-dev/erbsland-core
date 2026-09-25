// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Format.hpp"

#include "../StringCharReader.hpp"

#include "../../err/LogicError.hpp"
#include "../../util/Set.hpp"

namespace erbsland::text::named_key {

void Format::Key::validate() const {
    if (name.isEmpty()) {
        throw err::LogicError{"The name of a key must not be empty"};
    }
    if (name != name.transformed(Char::toIdentifierNormalized)) {
        throw err::LogicError{"The name of a key must be in its normalized form"};
    }
    if (index < 0) {
        throw err::LogicError{"A named-key index must not be negative"};
    }
}

auto Format::setKeys(const std::initializer_list<Key> keys) -> Format & {
    return setKeys(Keys{keys});
}

auto Format::setKeys(Keys keys) -> Format & {
    auto processedNames = util::Set<String>{};
    for (const auto &key : keys) {
        key.validate();
        if (processedNames.contains(key.name)) {
            throw err::LogicError{"A named-key alias must not be registered more than once"};
        }
        processedNames.insert(key.name);
    }
    _keys = std::move(keys);
    return *this;
}

auto Format::addKey(String name, const int index) -> Format & {
    auto key = Key{std::move(name), index};
    key.validate();
    for (const auto &testedKey : _keys) {
        if (testedKey.name == key.name) {
            throw err::LogicError{"A named-key alias must not be registered more than once"};
        }
    }
    _keys.append(std::move(key));
    return *this;
}

auto Format::keyIndex(const String &key) const -> std::optional<int> {
    const auto normalized = key.transformed(Char::toIdentifierNormalized);
    for (const auto &candidate : _keys) {
        if (candidate.name == normalized) {
            return candidate.index;
        }
    }
    return {};
}

auto Format::keyName(const int keyIndex) const -> const String & {
    for (const auto &key : _keys) {
        if (key.index == keyIndex) {
            return key.name;
        }
    }
    throw err::LogicError{"The named-key identifier is not registered"};
}

auto Format::setUniqueKeysRequired(const bool enabled) noexcept -> Format & {
    _uniqueKeysRequired = enabled;
    return *this;
}

auto Format::setKeysWithoutValuesAllowed(const bool enabled) noexcept -> Format & {
    _keysWithoutValuesAllowed = enabled;
    return *this;
}

auto Format::setValuesAllowed(const bool enabled) noexcept -> Format & {
    _valuesAllowed = enabled;
    return *this;
}

auto Format::setValueListAllowed(const bool enabled) noexcept -> Format & {
    _valueListAllowed = enabled;
    return *this;
}

auto Format::setMaximumValues(const unit::ItemCount maximum) noexcept -> Format & {
    _maximumValues = maximum;
    return *this;
}

auto Format::setMaximumValueLength(const unit::CpLength maximum) noexcept -> Format & {
    _maximumValueLength = maximum;
    return *this;
}

auto Format::setListSeparator(const Char separator) noexcept -> Format & {
    _listSeparator = separator;
    return *this;
}

auto Format::setValueSeparator(const Char separator) noexcept -> Format & {
    _valueSeparator = separator;
    return *this;
}

auto Format::setStopCharacter(const Char character) noexcept -> Format & {
    _stopCharacter = character;
    return *this;
}

auto Format::setAllowedKeyPrefixes(CharSet characters) noexcept -> Format & {
    _allowedKeyPrefixes = std::move(characters);
    return *this;
}

auto Format::setAllowedValueChars(CharSet characters) noexcept -> Format & {
    _allowedValueChars = std::move(characters);
    return *this;
}

auto Format::setValueWithoutKeySeparatorChars(CharSet characters) noexcept -> Format & {
    _valueWithoutKeySeparatorChars = std::move(characters);
    return *this;
}

void Format::validate() const {
    if (_listSeparator.isSignal() || !_listSeparator.isSafeUnicode()) {
        throw err::LogicError{"The named-key list separator must be a safe character"};
    }
    if (_valueSeparator.isSignal() || !_valueSeparator.isSafeUnicode()) {
        throw err::LogicError{"The named-key value separator must be a safe character"};
    }
    if (_listSeparator == _valueSeparator) {
        throw err::LogicError{"The named-key list and value separators must be different"};
    }
    if (!_stopCharacter.isEndOfData()) {
        if (_stopCharacter.isSignal() || !_stopCharacter.isSafeUnicode()) {
            throw err::LogicError{"The named-key stop character must be safe or end-of-data"};
        }
        if (_stopCharacter == _listSeparator || _stopCharacter == _valueSeparator) {
            throw err::LogicError{"The named-key stop character must be distinct from the separators"};
        }
    }
    for (const auto &key : _keys) {
        auto reader = StringCharReader{key.name};
        auto isFirst = true;
        while (!reader.isAtEnd()) {
            const auto character = reader.read();
            if (!character.isSafeUnicode() || character == _listSeparator || character == _valueSeparator ||
                character == _stopCharacter || _valueWithoutKeySeparatorChars.contains(character) ||
                (isFirst && _allowedKeyPrefixes.contains(character))) {
                throw err::LogicError{"A named-key alias contains a configured syntax character"};
            }
            isFirst = false;
        }
    }
    if (_allowedKeyPrefixes.contains(_listSeparator) || _allowedKeyPrefixes.contains(_valueSeparator) ||
        _allowedKeyPrefixes.contains(_stopCharacter) ||
        !_allowedKeyPrefixes.intersectedWith(_valueWithoutKeySeparatorChars).isEmpty()) {
        throw err::LogicError{"A named-key prefix conflicts with a configured syntax character"};
    }
    if (_valueWithoutKeySeparatorChars.contains(_listSeparator) ||
        _valueWithoutKeySeparatorChars.contains(_valueSeparator) ||
        _valueWithoutKeySeparatorChars.contains(_stopCharacter)) {
        throw err::LogicError{"A named-key compact-value character conflicts with a configured syntax character"};
    }
}

}
