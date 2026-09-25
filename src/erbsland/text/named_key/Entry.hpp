// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Entry_fwd.hpp"
#include "EntryKind.hpp"

#include "../Char.hpp"
#include "../String.hpp"

#include <cstdint>
#include <utility>

namespace erbsland::text::named_key {

/// One entry produced by `Parser`.
/// @tested{NamedKeyParserTest}
class Entry final {
public:
    /// Create the terminal entry.
    Entry() = default;
    /// Create an entry with all parsed fields.
    Entry(EntryKind kind, Char prefix, int keyIndex, String value) noexcept :
        _kind{kind}, _prefix{prefix}, _keyIndex{keyIndex}, _value{std::move(value)} {}

    // defaults
    ~Entry() = default;
    Entry(const Entry &) = default;
    Entry(Entry &&) = default;
    auto operator=(const Entry &) -> Entry & = default;
    auto operator=(Entry &&) -> Entry & = default;

public: // tests
    /// Test if this is a key without a value.
    [[nodiscard]] auto isKey() const noexcept -> bool { return _kind == EntryKind::Key; }
    /// Test if this is a key with a value.
    [[nodiscard]] auto isKeyWithValue() const noexcept -> bool { return _kind == EntryKind::KeyWithValue; }
    /// Test if this is a positional value.
    [[nodiscard]] auto isValue() const noexcept -> bool { return _kind == EntryKind::Value; }
    /// Test if this is the terminal entry.
    [[nodiscard]] auto isEnd() const noexcept -> bool { return _kind == EntryKind::End; }
    /// Test if this entry has a keyed or positional value.
    [[nodiscard]] auto hasValue() const noexcept -> bool {
        return _kind == EntryKind::KeyWithValue || _kind == EntryKind::Value;
    }

public: // accessors
    /// Get the entry kind.
    [[nodiscard]] auto kind() const noexcept -> EntryKind { return _kind; }
    /// Get the optional key prefix.
    [[nodiscard]] auto prefix() const noexcept -> Char { return _prefix; }
    /// Get the key identifier, or `-1` for non-key entries.
    [[nodiscard]] auto keyIndex() const noexcept -> int { return _keyIndex; }
    /// Get the value, or an empty string for entries without a value.
    [[nodiscard]] auto value() const noexcept -> const String & { return _value; }

private:
    EntryKind _kind{EntryKind::End};   ///< The entry kind.
    Char _prefix{Char::noCodePoint()}; ///< The optional key prefix.
    int _keyIndex{-1};                 ///< The semantic key identifier.
    String _value;                     ///< The captured UTF-8 value.
};

}
