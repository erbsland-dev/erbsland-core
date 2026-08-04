// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "NamedKeyEntry_fwd.hpp"
#include "NamedKeyEntryKind.hpp"

#include "../Char.hpp"
#include "../String.hpp"

#include <cstdint>
#include <utility>

namespace erbsland::text::impl {

/// One entry produced by `NamedKeyParser`.
/// @tested{NamedKeyParserTest}
class NamedKeyEntry final {
public:
    /// Create the terminal entry.
    NamedKeyEntry() = default;
    /// Create an entry with all parsed fields.
    NamedKeyEntry(NamedKeyEntryKind kind, Char prefix, int keyIndex, String value) noexcept :
        _kind{kind}, _prefix{prefix}, _keyIndex{keyIndex}, _value{std::move(value)} {}

    // defaults
    ~NamedKeyEntry() = default;
    NamedKeyEntry(const NamedKeyEntry &) = default;
    NamedKeyEntry(NamedKeyEntry &&) = default;
    auto operator=(const NamedKeyEntry &) -> NamedKeyEntry & = default;
    auto operator=(NamedKeyEntry &&) -> NamedKeyEntry & = default;

public: // tests
    /// Test if this is a key without a value.
    [[nodiscard]] auto isKey() const noexcept -> bool { return _kind == NamedKeyEntryKind::Key; }
    /// Test if this is a key with a value.
    [[nodiscard]] auto isKeyWithValue() const noexcept -> bool { return _kind == NamedKeyEntryKind::KeyWithValue; }
    /// Test if this is a positional value.
    [[nodiscard]] auto isValue() const noexcept -> bool { return _kind == NamedKeyEntryKind::Value; }
    /// Test if this is the terminal entry.
    [[nodiscard]] auto isEnd() const noexcept -> bool { return _kind == NamedKeyEntryKind::End; }
    /// Test if this entry has a keyed or positional value.
    [[nodiscard]] auto hasValue() const noexcept -> bool {
        return _kind == NamedKeyEntryKind::KeyWithValue || _kind == NamedKeyEntryKind::Value;
    }

public: // accessors
    /// Get the entry kind.
    [[nodiscard]] auto kind() const noexcept -> NamedKeyEntryKind { return _kind; }
    /// Get the optional key prefix.
    [[nodiscard]] auto prefix() const noexcept -> Char { return _prefix; }
    /// Get the key identifier, or `-1` for non-key entries.
    [[nodiscard]] auto keyIndex() const noexcept -> int { return _keyIndex; }
    /// Get the value, or an empty string for entries without a value.
    [[nodiscard]] auto value() const noexcept -> const String & { return _value; }

private:
    NamedKeyEntryKind _kind{NamedKeyEntryKind::End}; ///< The entry kind.
    Char _prefix{Char::noCodePoint()};               ///< The optional key prefix.
    int _keyIndex{-1};                               ///< The semantic key identifier.
    String _value;                                   ///< The captured UTF-8 value.
};

}
