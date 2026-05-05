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

    [[nodiscard]] constexpr auto firstYear() const noexcept -> uint16_t {
        return static_cast<uint16_t>(extractUnsigned(0, 14));
    }
    [[nodiscard]] constexpr auto yearLength() const noexcept -> uint8_t {
        return static_cast<uint8_t>(extractUnsigned(14, 8));
    }
    [[nodiscard]] constexpr auto lastYear() const noexcept -> uint16_t {
        const auto length = yearLength();
        return length == 0xffU ? uint16_t{9999U} : static_cast<uint16_t>(firstYear() + length);
    }
    [[nodiscard]] constexpr auto month() const noexcept -> uint8_t {
        return static_cast<uint8_t>(extractUnsigned(22, 4));
    }
    [[nodiscard]] constexpr auto day() const noexcept -> uint8_t {
        return static_cast<uint8_t>(extractUnsigned(26, 5));
    }
    [[nodiscard]] constexpr auto dayOfWeek() const noexcept -> uint8_t {
        return static_cast<uint8_t>(extractUnsigned(31, 3));
    }
    [[nodiscard]] constexpr auto dayRule() const noexcept -> DayRule {
        return static_cast<DayRule>(extractUnsigned(34, 2));
    }
    [[nodiscard]] constexpr auto atTime() const noexcept -> uint16_t {
        return static_cast<uint16_t>(extractUnsigned(36, 11));
    }
    [[nodiscard]] constexpr auto atTimeZone() const noexcept -> RuleAtTimeZone {
        return static_cast<RuleAtTimeZone>(extractUnsigned(47, 2));
    }
    [[nodiscard]] constexpr auto save() const noexcept -> int16_t {
        return static_cast<int16_t>(extractSigned(49, 15));
    }

    Data data;

private:
    [[nodiscard]] constexpr auto extractUnsigned(uint8_t shift, uint8_t width) const noexcept -> uint64_t {
        return (data >> shift) & ((uint64_t{1} << width) - 1U);
    }
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
