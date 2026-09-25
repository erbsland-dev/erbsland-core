// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Entry.hpp"
#include "Format.hpp"

#include "../StringCharReader.hpp"

#include "../../util/List.hpp"
#include "../../util/Set.hpp"

#include <optional>
#include <string_view>

namespace erbsland::text::named_key {

/// A low-level parser for named-key and positional-value entry lists.
/// All syntax errors are reported as positioned `err::ParseError` exceptions.
/// @tested{NamedKeyParserTest}
class Parser final {
    /// Identifies the entry mode selected by the first parsed entry.
    enum class Mode : uint8_t {
        Undetermined, ///< No entry kind has been selected.
        Keys,         ///< Named-key entries are active.
        Values,       ///< Positional values are active.
    };

public:
    /// Create a parser at the first entry.
    /// @throws err::LogicError if the format is invalid.
    Parser(StringCharReader &reader, const Format &format);

public:
    /// Get the active format.
    [[nodiscard]] auto format() const noexcept -> const Format & { return _format; }
    /// Restrict recognized keys to the given semantic identifiers.
    void setAllowedKeys(util::Set<int> keys);
    /// Read one entry or the terminal entry.
    /// @throws err::ParseError for invalid syntax.
    [[nodiscard]] auto readEntry() -> Entry;
    /// Read all remaining entries, excluding the terminal entry.
    /// @throws err::ParseError for invalid syntax.
    [[nodiscard]] auto readAllEntries() -> util::List<Entry>;

private:
    /// Test if the reader is at the list terminator.
    [[nodiscard]] auto isAtTerminator() const noexcept -> bool;
    /// Read and validate the terminal entry.
    [[nodiscard]] auto readEnd() -> Entry;
    /// Capture one raw encoded entry.
    [[nodiscard]] auto captureRawEntry() -> String;
    /// Parse one raw entry according to the active mode.
    [[nodiscard]] auto parseRawEntry(const String &raw, unit::CpIndex position) -> Entry;
    /// Parse a named-key entry.
    [[nodiscard]] auto parseKeyEntry(StringCharReader &reader, Char prefix, int keyIndex, unit::CpIndex position)
        -> Entry;
    /// Parse a positional-value entry.
    [[nodiscard]] auto parsePositionalValue(const String &raw, unit::CpIndex position) -> Entry;
    /// Validate a decoded entry value.
    void validateValue(const String &value, unit::CpIndex position) const;
    /// Record a key as seen or report a duplicate.
    void markKeySeen(int keyIndex, unit::CpIndex position);
    /// Throw a positioned parse error.
    [[noreturn]] static void throwError(std::string_view reason, unit::CpIndex position);

private:
    StringCharReader &_reader;
    const Format &_format;
    std::optional<util::Set<int>> _allowedKeys;
    util::Set<int> _seenKeys;
    Mode _mode{Mode::Undetermined};
    unit::ItemCount _valueCount{};
    bool _firstEntry{true};
    bool _ended{false};
};

}
