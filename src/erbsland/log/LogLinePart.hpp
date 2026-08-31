// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../text/String_fwd.hpp"
#include "../util/impl/ComparisonHelper.hpp"

#include <cstdint>
#include <optional>

namespace erbsland::log {

/// The semantic role of a formatted line segment.
/// @tested{LogCoreTest}
class LogLinePart final {
public:
    /// The raw line-part value.
    enum Value : uint8_t {
        Literal, ///< Literal text from the format pattern.
        Time,    ///< Rendered timestamp.
        Level,   ///< Rendered severity level.
        Name,    ///< Rendered stream path.
        Message, ///< Rendered message.
        _Count,  ///< Number of semantic parts.
    };

    /// Create the default literal part.
    constexpr LogLinePart() noexcept = default;
    /// Create a line part from its raw value.
    /// @param value The raw semantic-part value.
    constexpr LogLinePart(const Value value) noexcept : _value{value} {} // NOLINT(*-explicit-constructor)

public:                                                                  // operators
    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_value, const LogLinePart &other, other._value);
    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_value, const Value value, value);
    ERBSLAND_CORE_CONSTEXPR_COMPARE_FRIEND(const Value value, const LogLinePart &other, value, other._value);

public: // accessors
    /// Get the raw semantic-part value.
    /// @return The embedded value.
    [[nodiscard]] constexpr auto toRawValue() const noexcept -> Value { return _value; }

public: // conversion
    /// Convert this part to its placeholder identifier.
    /// @return The lowercase placeholder name, or an empty string for a literal or invalid part.
    [[nodiscard]] auto toString() const -> text::String;
    /// Parse a placeholder identifier.
    /// @param text The placeholder name without braces.
    /// @return The corresponding semantic part, or an empty optional if the name is unknown.
    [[nodiscard]] static auto fromString(const text::String &text) noexcept -> std::optional<LogLinePart>;
    /// Parse a placeholder identifier.
    /// @param text The placeholder name without braces.
    /// @return The corresponding semantic part.
    /// @throws err::ParseError If the placeholder name is unknown.
    [[nodiscard]] static auto fromStringOrThrow(const text::String &text) -> LogLinePart;

private:
    Value _value{Literal}; ///< Raw semantic-part value.
};

}
