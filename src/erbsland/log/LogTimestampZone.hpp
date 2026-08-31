// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../text/String_fwd.hpp"
#include "../text/StringList_fwd.hpp"
#include "../util/impl/ComparisonHelper.hpp"

#include <cstdint>
#include <optional>

namespace erbsland::log {

/// The time zone used when rendering an entry timestamp.
/// @tested{LogCoreTest}
class LogTimestampZone final {
public:
    /// The raw timestamp-zone value.
    enum Value : uint8_t {
        Utc,   ///< Render the retained UTC timestamp.
        Local, ///< Convert to the local time zone while rendering.
    };

    /// Create the default UTC timestamp zone.
    constexpr LogTimestampZone() noexcept = default;
    /// Create a timestamp zone from its raw value.
    /// @param value The raw timestamp-zone value.
    constexpr LogTimestampZone(const Value value) noexcept : _value{value} {} // NOLINT(*-explicit-constructor)

public:                                                                       // operators
    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_value, const LogTimestampZone &other, other._value);
    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_value, const Value value, value);
    ERBSLAND_CORE_CONSTEXPR_COMPARE_FRIEND(const Value value, const LogTimestampZone &other, value, other._value);

public: // accessors
    /// Get the raw timestamp-zone value.
    /// @return The embedded value.
    [[nodiscard]] constexpr auto toRawValue() const noexcept -> Value { return _value; }

public: // conversion
    /// Convert this zone to its canonical configuration identifier.
    /// @return `utc` or `local`.
    [[nodiscard]] auto toString() const -> text::String;
    /// Get all canonical configuration identifiers.
    /// @return The identifiers accepted by `fromString()`, irrespective of ASCII letter case.
    [[nodiscard]] static auto allStrings() -> text::StringList;
    /// Parse a canonical timestamp-zone identifier case-insensitively using ASCII folding.
    /// @param text The identifier to parse.
    /// @return The parsed zone, or an empty optional if the identifier is unknown.
    [[nodiscard]] static auto fromString(const text::String &text) noexcept -> std::optional<LogTimestampZone>;
    /// Parse a canonical timestamp-zone identifier.
    /// @param text The identifier to parse.
    /// @return The parsed zone.
    /// @throws err::ParseError If the identifier is unknown.
    [[nodiscard]] static auto fromStringOrThrow(const text::String &text) -> LogTimestampZone;

private:
    Value _value{Utc}; ///< Raw timestamp-zone value.
};

}
