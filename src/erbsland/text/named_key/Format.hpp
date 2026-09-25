// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Format_fwd.hpp"

#include "../Char.hpp"
#include "../CharSet.hpp"
#include "../String.hpp"

#include "../../unit/CpLength.hpp"
#include "../../unit/ItemCount.hpp"
#include "../../util/List.hpp"

#include <initializer_list>
#include <optional>
#include <utility>

namespace erbsland::text::named_key {

/// The grammar and key table for `Parser`.
/// @tested{NamedKeyParserTest}
class Format final {
public:
    /// One normalized key alias and its semantic identifier.
    /// @tested{NamedKeyParserTest}
    struct Key final {
        String name; ///< The key alias.
        int index{}; ///< The semantic key identifier.

        /// Validate this key entry.
        void validate() const;
    };
    /// The list of key aliases.
    using Keys = util::List<Key>;

public:
    /// Create the default named-key format.
    Format() = default;

    // defaults
    ~Format() = default;
    Format(const Format &) = default;
    Format(Format &&) = default;
    auto operator=(const Format &) -> Format & = default;
    auto operator=(Format &&) -> Format & = default;

public: // key table
    /// Replace all key aliases.
    auto setKeys(std::initializer_list<Key> keys) -> Format &;
    /// @overload
    auto setKeys(Keys keys) -> Format &;
    /// Add one key alias.
    auto addKey(String name, int index) -> Format &;
    /// Find a semantic key identifier from any supported spelling.
    [[nodiscard]] auto keyIndex(const String &key) const -> std::optional<int>;
    /// Get the first registered normalized alias for a semantic key identifier.
    /// @throws err::LogicError if the identifier is not registered.
    [[nodiscard]] auto keyName(int keyIndex) const -> const String &;
    /// Get all registered key aliases.
    [[nodiscard]] auto keys() const noexcept -> const util::List<Key> & { return _keys; }

public: // policy
    /// Require each semantic key at most once.
    auto setUniqueKeysRequired(bool enabled) noexcept -> Format &;
    /// Test if each semantic key is limited to one occurrence.
    [[nodiscard]] auto uniqueKeysRequired() const noexcept -> bool { return _uniqueKeysRequired; }
    /// Allow recognized keys without values.
    auto setKeysWithoutValuesAllowed(bool enabled) noexcept -> Format &;
    /// Test if recognized keys without values are allowed.
    [[nodiscard]] auto keysWithoutValuesAllowed() const noexcept -> bool { return _keysWithoutValuesAllowed; }
    /// Allow values attached to recognized keys.
    auto setValuesAllowed(bool enabled) noexcept -> Format &;
    /// Test if values attached to recognized keys are allowed.
    [[nodiscard]] auto valuesAllowed() const noexcept -> bool { return _valuesAllowed; }
    /// Allow a positional value list.
    auto setValueListAllowed(bool enabled) noexcept -> Format &;
    /// Test if a positional value list is allowed.
    [[nodiscard]] auto valueListAllowed() const noexcept -> bool { return _valueListAllowed; }
    /// Set the maximum number of positional values.
    auto setMaximumValues(unit::ItemCount maximum) noexcept -> Format &;
    /// Get the maximum number of positional values.
    [[nodiscard]] auto maximumValues() const noexcept -> unit::ItemCount { return _maximumValues; }
    /// Set the maximum value length in code points.
    auto setMaximumValueLength(unit::CpLength maximum) noexcept -> Format &;
    /// Get the maximum value length in code points.
    [[nodiscard]] auto maximumValueLength() const noexcept -> unit::CpLength { return _maximumValueLength; }

public: // syntax
    /// Set the entry-list separator.
    auto setListSeparator(Char separator) noexcept -> Format &;
    /// Get the entry-list separator.
    [[nodiscard]] auto listSeparator() const noexcept -> Char { return _listSeparator; }
    /// Set the separator between a key and its value.
    auto setValueSeparator(Char separator) noexcept -> Format &;
    /// Get the separator between a key and its value.
    [[nodiscard]] auto valueSeparator() const noexcept -> Char { return _valueSeparator; }
    /// Set the required stop character, or end-of-data for an end-terminated list.
    auto setStopCharacter(Char character) noexcept -> Format &;
    /// Get the required stop character.
    [[nodiscard]] auto stopCharacter() const noexcept -> Char { return _stopCharacter; }
    /// Set the optional key prefix characters.
    auto setAllowedKeyPrefixes(CharSet characters) noexcept -> Format &;
    /// Get the optional key prefix characters.
    [[nodiscard]] auto allowedKeyPrefixes() const noexcept -> const CharSet & { return _allowedKeyPrefixes; }
    /// Set the allowed value characters. An empty set permits every safe character.
    auto setAllowedValueChars(CharSet characters) noexcept -> Format &;
    /// Get the allowed value characters.
    [[nodiscard]] auto allowedValueChars() const noexcept -> const CharSet & { return _allowedValueChars; }
    /// Set characters that start a compact value immediately after a key.
    auto setValueWithoutKeySeparatorChars(CharSet characters) noexcept -> Format &;
    /// Get characters that start a compact value immediately after a key.
    [[nodiscard]] auto valueWithoutKeySeparatorChars() const noexcept -> const CharSet & {
        return _valueWithoutKeySeparatorChars;
    }

public: // validation
    /// Validate this format before parsing.
    /// @throws err::LogicError if the format is ambiguous or invalid.
    void validate() const;

private:
    Keys _keys;                                                  ///< Normalized key aliases.
    bool _uniqueKeysRequired{true};                              ///< Require unique semantic keys.
    bool _keysWithoutValuesAllowed{true};                        ///< Allow bare keys.
    bool _valuesAllowed{true};                                   ///< Allow keyed values.
    bool _valueListAllowed{true};                                ///< Allow positional values.
    unit::ItemCount _maximumValues{unit::ItemCount::infinite()}; ///< Maximum positional value count.
    unit::CpLength _maximumValueLength{200U};                    ///< Maximum value length.
    Char _listSeparator{U','};                                   ///< Entry-list separator.
    Char _valueSeparator{U'='};                                  ///< Key/value separator.
    Char _stopCharacter{Char::endOfData()};                      ///< Required stop character.
    CharSet _allowedKeyPrefixes;                                 ///< Optional key prefixes.
    CharSet _allowedValueChars;                                  ///< Allowed value characters.
    CharSet _valueWithoutKeySeparatorChars;                      ///< Compact-value start characters.
};

}
