// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

namespace erbsland::time {

template <typename tUnit>
    requires impl::DateTimeTickUnit<tUnit>
auto DateTime::toTicks(const TimeEpoch epoch) const noexcept -> std::optional<tUnit> {
    const auto parts = toSecondsAndFractions(epoch);
    if (!parts.has_value()) {
        return std::nullopt;
    }
    const auto &[seconds, fractions] = *parts;
    if constexpr (std::same_as<tUnit, Seconds>) {
        return seconds;
    } else {
        if (seconds.template wouldConvertSaturate<tUnit>()) {
            return std::nullopt;
        }
        const auto result = seconds.template converted<tUnit>();
        const auto fractionTicks = fractions.template converted<tUnit>();
        if (result.wouldAddSaturate(fractionTicks)) {
            return std::nullopt;
        }
        return result + fractionTicks;
    }
}

template <typename tUnit>
    requires impl::DateTimeTickUnit<tUnit>
auto DateTime::toTicksOrThrow(const TimeEpoch epoch) const -> tUnit {
    const auto result = toTicks<tUnit>(epoch);
    if (!result.has_value()) {
        throwDateTimeNotTickConvertible();
    }
    return *result;
}

template <typename tUnit>
    requires impl::DateTimeTickUnit<tUnit>
auto DateTime::fromTicks(tUnit ticks, const TimeEpoch epoch) noexcept -> std::optional<DateTime> {
    if (ticks.isNegative()) {
        return std::nullopt;
    }
    if constexpr (std::same_as<tUnit, Seconds>) {
        return fromTicks(ticks, Nanoseconds{}, epoch);
    } else {
        const auto seconds = ticks.template extract<Seconds>();
        const auto fractions = ticks.template converted<Nanoseconds>();
        return fromTicks(seconds, fractions, epoch);
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
