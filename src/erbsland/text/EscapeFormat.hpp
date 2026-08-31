// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "StringEditor_fwd.hpp"

#include "../util/impl/ComparisonHelper.hpp"

#include <array>
#include <cstdint>
#include <optional>

namespace erbsland::text {

/// The target syntax for escaped text.
/// @tested{StringEscapingTest}
class EscapeFormat final {
public:
    /// The escape format value.
    enum Value : uint8_t {
        None,        ///< Do not escape any characters.
        Html,        ///< Escape for HTML text.
        Json,        ///< Escape for JSON text.
        Cpp,         ///< Escape for C++ literals.
        Xml,         ///< Escape for XML text.
        RegEx,       ///< Escape for regular expression literal patterns.
        Display,     ///< Escape unsafe characters for human-readable display text (equals Config).
        Log,         ///< Escape unsafe log text while preserving line feeds.
        Config,      ///< Escape for Erbsland Configuration Language text literals.
        ConfigTest,  ///< Escape for Erbsland Configuration Language test strings.
        Markdown,    ///< Escape normal CommonMark text using backslash and numeric references.

        _valueCount, ///< Number of escape formats.
    };

private:
    using ValueToTextArray = std::array<std::pair<Value, StringLiteral>, _valueCount>;

public:
    /// Create an escape format from a value.
    /// @param value The raw escape-format value.
    constexpr EscapeFormat(const Value value) noexcept : _value{value} {} // NOLINT(*-explicit-constructor)

    // defaults
    constexpr EscapeFormat() noexcept = default;
    ~EscapeFormat() = default;
    EscapeFormat(const EscapeFormat &) = default;
    EscapeFormat(EscapeFormat &&) = default;
    auto operator=(const EscapeFormat &) -> EscapeFormat & = default;
    auto operator=(EscapeFormat &&) -> EscapeFormat & = default;

public: // operators
    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_value, const EscapeFormat &other, other._value);
    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_value, const Value value, value);
    ERBSLAND_CORE_CONSTEXPR_COMPARE_FRIEND(const Value value, const EscapeFormat &other, value, other._value);

public: // accessors
    /// Get the raw escape format value.
    [[nodiscard]] constexpr auto toRawValue() const noexcept -> Value { return _value; }

public: // conversion
    /// Convert this escape format to its canonical string.
    /// @return The lowercase canonical identifier for this format.
    [[nodiscard]] auto toString() const -> String;
    /// Create an escape format from a canonical string.
    /// @param text The canonical identifier to parse.
    /// @return The parsed format, or an empty optional if `text` is unknown.
    [[nodiscard]] static auto fromString(const String &text) noexcept -> std::optional<EscapeFormat>;
    /// Create an escape format from a canonical string.
    /// @param text The canonical identifier to parse.
    /// @return The parsed escape format.
    /// @throws err::ParseError if the string is not a supported escape format.
    [[nodiscard]] static auto fromStringOrThrow(const String &text) -> EscapeFormat;

private:
    static const ValueToTextArray _valueToTextMap; ///< The value to text map.
    Value _value{None};                            ///< The escape format value.
};

}
