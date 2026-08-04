// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "DayRule.hpp"
#include "RuleAtTimeZone.hpp"

#include <cstdint>

namespace erbsland::time::tz::impl {

/// A packed generated rule entry.
/// @notest{Internal generated-data helper.}
struct Rule final {
    using Data = uint64_t;

    /// Get the first year covered by the rule.
    /// @return The first covered year.
    [[nodiscard]] constexpr auto firstYear() const noexcept -> uint16_t {
        return static_cast<uint16_t>(extractUnsigned(0, 14));
    }
    /// Get the count of years after the first year covered by the rule.
    /// @return The covered year range length.
    [[nodiscard]] constexpr auto yearLength() const noexcept -> uint8_t {
        return static_cast<uint8_t>(extractUnsigned(14, 8));
    }
    /// Get the final year covered by the rule.
    /// @return The final covered year.
    [[nodiscard]] constexpr auto lastYear() const noexcept -> uint16_t {
        const auto length = yearLength();
        return length == 0xffU ? uint16_t{9999U} : static_cast<uint16_t>(firstYear() + length);
    }
    /// Get the month selected by the rule.
    /// @return The one-based month number.
    [[nodiscard]] constexpr auto month() const noexcept -> uint8_t {
        return static_cast<uint8_t>(extractUnsigned(22, 4));
    }
    /// Get the day value selected by the rule.
    /// @return The rule day value.
    [[nodiscard]] constexpr auto day() const noexcept -> uint8_t {
        return static_cast<uint8_t>(extractUnsigned(26, 5));
    }
    /// Get the weekday selected by the rule.
    /// @return The weekday number.
    [[nodiscard]] constexpr auto dayOfWeek() const noexcept -> uint8_t {
        return static_cast<uint8_t>(extractUnsigned(31, 3));
    }
    /// Get how the rule day value is interpreted.
    /// @return The rule-day interpretation.
    [[nodiscard]] constexpr auto dayRule() const noexcept -> DayRule {
        return static_cast<DayRule>(extractUnsigned(34, 2));
    }
    /// Get the time of day selected by the rule.
    /// @return The encoded time of day.
    [[nodiscard]] constexpr auto atTime() const noexcept -> uint16_t {
        return static_cast<uint16_t>(extractUnsigned(36, 11));
    }
    /// Get the reference used for the rule time of day.
    /// @return The rule-time reference.
    [[nodiscard]] constexpr auto atTimeZone() const noexcept -> RuleAtTimeZone {
        return static_cast<RuleAtTimeZone>(extractUnsigned(47, 2));
    }
    /// Get the daylight-saving offset adjustment.
    /// @return The offset adjustment in seconds.
    [[nodiscard]] constexpr auto save() const noexcept -> int16_t {
        return static_cast<int16_t>(extractSigned(49, 15));
    }

    Data data;

private:
    /// Extract an unsigned bit field from the packed entry.
    /// @param shift The bit-field start position.
    /// @param width The bit-field width.
    /// @return The extracted unsigned value.
    [[nodiscard]] constexpr auto extractUnsigned(uint8_t shift, uint8_t width) const noexcept -> uint64_t {
        return (data >> shift) & ((uint64_t{1} << width) - 1U);
    }
    /// Extract a signed bit field from the packed entry.
    /// @param shift The bit-field start position.
    /// @param width The bit-field width.
    /// @return The extracted signed value.
    [[nodiscard]] constexpr auto extractSigned(uint8_t shift, uint8_t width) const noexcept -> int64_t {
        const auto raw = extractUnsigned(shift, width);
        const auto signBit = uint64_t{1} << (width - 1U);
        if ((raw & signBit) == 0U) {
            return static_cast<int64_t>(raw);
        }
        return static_cast<int64_t>(raw | (~((uint64_t{1} << width) - 1U)));
    }
};

}
