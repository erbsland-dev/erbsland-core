// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../err/OutOfRangeError.hpp"

namespace erbsland::time {

template <typename tUnit>
    requires impl::DateTimeTickUnit<tUnit>
auto DateTime::toTicks(const TimeEpoch epoch) const noexcept -> std::optional<tUnit> {
    try {
        return toTicksOrThrow<tUnit>(epoch);
    } catch (const err::Exception &) {
        return std::nullopt;
    }
}

template <typename tUnit>
    requires impl::DateTimeTickUnit<tUnit>
auto DateTime::toTicksOrThrow(const TimeEpoch epoch) const -> tUnit {
    const auto &[seconds, fractions] = toSecondsAndFractionsOrThrow(epoch);
    if constexpr (std::same_as<tUnit, Seconds>) {
        return seconds;
    } else {
        if (seconds.template wouldConvertSaturate<tUnit>()) {
            throwDateTimeNotTickConvertible();
        }
        auto result = seconds.template converted<tUnit>();
        const auto fractionTicks = fractions.template converted<tUnit>();
        if (result.wouldAddSaturate(fractionTicks)) [[unlikely]] {
            throwDateTimeNotTickConvertible();
        }
        result += fractionTicks;
        return result;
    }
}

template <typename tUnit>
    requires impl::DateTimeTickUnit<tUnit>
auto DateTime::fromTicks(const tUnit ticks, const TimeEpoch epoch) noexcept -> std::optional<DateTime> {
    try {
        return fromTicksOrThrow<tUnit>(ticks, epoch);
    } catch (const err::Exception &) {
        return std::nullopt;
    }
}

template <typename tUnit>
    requires impl::DateTimeTickUnit<tUnit>
auto DateTime::fromTicksOrThrow(tUnit ticks, const TimeEpoch epoch) -> DateTime {
    if (ticks.isNegative()) {
        throwTicksMustNotBeNegative();
    }
    if constexpr (std::same_as<tUnit, Seconds>) {
        return fromTicksOrThrow(ticks, Nanoseconds{}, epoch);
    } else if constexpr (std::same_as<tUnit, Nanoseconds>) {
        const auto seconds = ticks.template extract<Seconds>();
        return fromTicksOrThrow(seconds, ticks, epoch);
    } else {
        const auto seconds = ticks.template extract<Seconds>();
        const auto fractions = ticks.template converted<Nanoseconds>();
        return fromTicksOrThrow(seconds, fractions, epoch);
    }
}

}
