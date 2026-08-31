// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../text/String_fwd.hpp"
#include "../text/StringList_fwd.hpp"
#include "../util/impl/ComparisonHelper.hpp"

#include <cstdint>
#include <optional>

namespace erbsland::log {

/// The rule used to limit rendered message text.
/// @tested{LogCoreTest}
class LogMessageTruncation final {
public:
    /// The raw message-truncation value.
    enum Value : uint8_t {
        None,            ///< Do not truncate while rendering.
        FirstLine,       ///< Keep only the first message line.
        CharacterCount,  ///< Limit the message by character count.
        TotalLineLength, ///< Limit the complete formatted line length.
    };

    /// Create the default no-truncation rule.
    constexpr LogMessageTruncation() noexcept = default;
    /// Create a truncation rule from its raw value.
    /// @param value The raw message-truncation value.
    constexpr LogMessageTruncation(const Value value) noexcept : _value{value} {} // NOLINT(*-explicit-constructor)

public:                                                                           // operators
    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_value, const LogMessageTruncation &other, other._value);
    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_value, const Value value, value);
    ERBSLAND_CORE_CONSTEXPR_COMPARE_FRIEND(const Value value, const LogMessageTruncation &other, value, other._value);

public: // accessors
    /// Get the raw message-truncation value.
    /// @return The embedded value.
    [[nodiscard]] constexpr auto toRawValue() const noexcept -> Value { return _value; }

public: // conversion
    /// Convert this rule to its canonical configuration identifier.
    /// @return The lowercase truncation identifier.
    [[nodiscard]] auto toString() const -> text::String;
    /// Get all canonical configuration identifiers.
    /// @return The identifiers accepted by `fromString()`, irrespective of ASCII letter case.
    [[nodiscard]] static auto allStrings() -> text::StringList;
    /// Parse a canonical message-truncation identifier case-insensitively using ASCII folding.
    /// @param text The identifier to parse.
    /// @return The parsed rule, or an empty optional if the identifier is unknown.
    [[nodiscard]] static auto fromString(const text::String &text) noexcept -> std::optional<LogMessageTruncation>;
    /// Parse a canonical message-truncation identifier.
    /// @param text The identifier to parse.
    /// @return The parsed rule.
    /// @throws err::ParseError If the identifier is unknown.
    [[nodiscard]] static auto fromStringOrThrow(const text::String &text) -> LogMessageTruncation;

private:
    Value _value{None}; ///< Raw message-truncation value.
};

}
