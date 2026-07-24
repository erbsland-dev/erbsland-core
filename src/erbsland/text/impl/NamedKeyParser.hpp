// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "NamedKeyEntry.hpp"
#include "NamedKeyFormat.hpp"

#include "../StringCharReader.hpp"

#include "../../util/List.hpp"
#include "../../util/Set.hpp"

#include <optional>
#include <string_view>

namespace erbsland::text::impl {

/// A low-level parser for named-key and positional-value entry lists.
/// All syntax errors are reported as positioned `err::ParseError` exceptions.
/// @tested{NamedKeyParserTest}
class NamedKeyParser final {
    enum class Mode : uint8_t {
        Undetermined, ///< No entry kind has been selected.
        Keys,         ///< Named-key entries are active.
        Values,       ///< Positional values are active.
    };

public:
    /// Create a parser at the first entry.
    /// @throws err::LogicError if the format is invalid.
    NamedKeyParser(StringCharReader &reader, const NamedKeyFormat &format);

public:
    /// Get the active format.
    [[nodiscard]] auto format() const noexcept -> const NamedKeyFormat & { return _format; }
    /// Restrict recognized keys to the given semantic identifiers.
    void setAllowedKeys(util::Set<int> keys);
    /// Read one entry or the terminal entry.
    /// @throws err::ParseError for invalid syntax.
    [[nodiscard]] auto readEntry() -> NamedKeyEntry;
    /// Read all remaining entries, excluding the terminal entry.
    /// @throws err::ParseError for invalid syntax.
    [[nodiscard]] auto readAllEntries() -> util::List<NamedKeyEntry>;

private:
    [[nodiscard]] auto isAtTerminator() const noexcept -> bool;
    [[nodiscard]] auto readEnd() -> NamedKeyEntry;
    [[nodiscard]] auto captureRawEntry() -> String;
    [[nodiscard]] auto parseRawEntry(const String &raw, unit::CpIndex position) -> NamedKeyEntry;
    [[nodiscard]] auto parseKeyEntry(StringCharReader &reader, Char prefix, int keyIndex, unit::CpIndex position)
        -> NamedKeyEntry;
    [[nodiscard]] auto parsePositionalValue(const String &raw, unit::CpIndex position) -> NamedKeyEntry;
    void validateValue(const String &value, unit::CpIndex position) const;
    void markKeySeen(int keyIndex, unit::CpIndex position);
    [[noreturn]] static void throwError(std::string_view reason, unit::CpIndex position);

private:
    StringCharReader &_reader;
    const NamedKeyFormat &_format;
    std::optional<util::Set<int>> _allowedKeys;
    util::Set<int> _seenKeys;
    Mode _mode{Mode::Undetermined};
    unit::ElementCount _valueCount{};
    bool _firstEntry{true};
    bool _ended{false};
};

}
