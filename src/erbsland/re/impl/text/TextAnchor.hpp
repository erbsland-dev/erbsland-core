// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../text/String.hpp"

#include <cstdint>
#include <vector>

namespace erbsland::re::impl {

/// A text anchor.
class TextAnchor {
public:
    enum Value : uint8_t {
        None = 0x00,                   // For error handling.
        Start = 0x01,                  ///< \\A or ^ in single-line mode.
        End = 0x02,                    ///< \\Z, \\z or $ in single-line mode.
        LineStart = 0x03,              ///< ^ in multi-line mode. Matches after `\n` or at the start of the text.
        LineEnd = 0x04,                ///< $ in multi-line mode. Matches before `\n` or at the end of the text.
        UnicodeWordBoundary = 0x05,    ///< \\b in Unicode mode
        AsciiWordBoundary = 0x06,      ///< \\b in ASCII mode
        NonUnicodeWordBoundary = 0x07, ///< \\B in Unicode mode
        NonAsciiWordBoundary = 0x08,   ///< \\B in ASCII mode
    };

public:
    /// Create a new anchor from the given enum value.
    TextAnchor(const Value value) : _value{value} {} // NOLINT(*-explicit-constructor)

    // defaults
    TextAnchor() = default;
    ~TextAnchor() = default;
    TextAnchor(const TextAnchor &) = default;
    auto operator=(const TextAnchor &) -> TextAnchor & = default;

public: // operators
    /// Compare two anchors for equality.
    [[nodiscard]] auto operator==(const TextAnchor &other) const noexcept -> bool { return _value == other._value; }
    /// Compare two anchors for inequality.
    [[nodiscard]] auto operator!=(const TextAnchor &other) const noexcept -> bool { return _value != other._value; }

public:
    /// Return a string for the given anchor. Mainly used for testing.
    [[nodiscard]] auto toString() const -> text::String;

    /// Return the raw value.
    [[nodiscard]] auto raw() const noexcept -> Value { return _value; }

    /// Create a new text anchor from the given string.
    /// @param str The string representation of the anchor.
    /// @return The created text anchor or None if the string is invalid.
    /// @throws err::ParameterError if the string is invalid.
    [[nodiscard]] static auto fromString(const text::String &str) -> TextAnchor;

private:
    using ValueToNameList = std::vector<std::pair<Value, text::String>>;
    /// Get the mapping from anchor values to their names.
    [[nodiscard]] static auto valueToNameList() noexcept -> const ValueToNameList &;

private:
    Value _value = None;
};

}
