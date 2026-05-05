// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../util/impl/ComparisonHelper.hpp"

#include <compare>
#include <cstdint>

namespace erbsland::time {

/// A transient identifier for a named time zone.
///
/// Used internally to reference time zones without storing full name strings.
/// @tested{TimeZoneTest}
class TimeZoneId final {
public:
    /// Create the UTC identifier.
    constexpr TimeZoneId() noexcept = default;
    /// Create an identifier from its raw value.
    /// @param value The raw identifier value.
    explicit constexpr TimeZoneId(uint16_t value) noexcept : _value{value} {}

    // defaults
    ~TimeZoneId() = default;
    TimeZoneId(const TimeZoneId &) noexcept = default;
    auto operator=(const TimeZoneId &) noexcept -> TimeZoneId & = default;

public:
    /// Compare identifiers.
    [[nodiscard]] constexpr auto operator<=>(const TimeZoneId &other) const noexcept -> std::strong_ordering = default;
    /// Return the raw identifier.
    /// @return The underlying identifier value.
    [[nodiscard]] constexpr auto toRawValue() const noexcept -> uint16_t { return _value; }
    /// Test if this is the UTC/no-zone identifier.
    /// @return `true` if this represents UTC.
    [[nodiscard]] constexpr auto isUtc() const noexcept -> bool { return _value == 0; }

private:
    uint16_t _value{0}; ///< The raw value.
};

}
