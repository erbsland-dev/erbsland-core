// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../err/OutOfRangeError.hpp"
#include "../err/OverflowError.hpp"
#include "../text/Literals.hpp"

namespace erbsland::time {

using namespace text::literals;

template <impl::TimestampAmount T>
auto Timestamp::splitAmount(T amount) noexcept -> std::pair<int64_t, int64_t> {
    if constexpr (std::same_as<T, Duration>) {
        return splitAmount(amount.toSeconds());
    } else if constexpr (std::same_as<T, TimeDelta>) {
        return splitAmount(amount.toNanoseconds());
    } else {
        constexpr auto cNanosecondsPerTick = cNanosecondsPerSecond / T::Ratio::den;
        constexpr auto cTicksPerDay = cNanosecondsPerDay / cNanosecondsPerTick;
        const auto ticks = amount.toRawValue();
        return {ticks / cTicksPerDay, (ticks % cTicksPerDay) * cNanosecondsPerTick};
    }
}

template <impl::TimestampAmount T>
auto Timestamp::wouldAddSaturate(T amount) const noexcept -> bool {
    const auto [days, nanoseconds] = splitAmount(amount);
    auto overflow = false;
    [[maybe_unused]] const auto result = shifted(days, nanoseconds, false, overflow);
    return overflow;
}

template <impl::TimestampAmount T>
auto Timestamp::added(T amount) const noexcept -> Timestamp {
    const auto [days, nanoseconds] = splitAmount(amount);
    auto overflow = false;
    return shifted(days, nanoseconds, false, overflow);
}

template <impl::TimestampAmount T>
auto Timestamp::addedOrThrow(T amount) const -> Timestamp {
    const auto [days, nanoseconds] = splitAmount(amount);
    auto overflow = false;
    const auto result = shifted(days, nanoseconds, false, overflow);
    if (overflow) {
        throw err::OverflowError{"Timestamp arithmetic exceeds the calendar range"_el};
    }
    return result;
}

template <impl::TimestampAmount T>
auto Timestamp::wouldSubtractSaturate(T amount) const noexcept -> bool {
    const auto [days, nanoseconds] = splitAmount(amount);
    auto overflow = false;
    [[maybe_unused]] const auto result = shifted(days, nanoseconds, true, overflow);
    return overflow;
}

template <impl::TimestampAmount T>
auto Timestamp::subtracted(T amount) const noexcept -> Timestamp {
    const auto [days, nanoseconds] = splitAmount(amount);
    auto overflow = false;
    return shifted(days, nanoseconds, true, overflow);
}

template <impl::TimestampAmount T>
auto Timestamp::subtractedOrThrow(T amount) const -> Timestamp {
    const auto [days, nanoseconds] = splitAmount(amount);
    auto overflow = false;
    const auto result = shifted(days, nanoseconds, true, overflow);
    if (overflow) {
        throw err::OverflowError{"Timestamp arithmetic exceeds the calendar range"_el};
    }
    return result;
}

template <impl::DateTimeTickUnit T>
auto Timestamp::toTicks(TimeEpoch epochValue) const noexcept -> std::optional<T> {
    if (!isValid()) {
        return std::nullopt;
    }
    auto overflow = false;
    constexpr auto cNanosecondsPerTick = cNanosecondsPerSecond / T::Ratio::den;
    const auto ticks = epoch(epochValue).distanceTicks(*this, cNanosecondsPerTick, overflow);
    if (overflow) {
        return std::nullopt;
    }
    return T{ticks};
}

template <impl::DateTimeTickUnit T>
auto Timestamp::toTicksOrThrow(TimeEpoch epochValue) const -> T {
    const auto result = toTicks<T>(epochValue);
    if (!result) {
        throw err::OutOfRangeError{"Timestamp cannot be represented in these epoch ticks"_el};
    }
    return *result;
}

template <impl::DateTimeTickUnit T>
auto Timestamp::fromTicks(T ticks, TimeEpoch epochValue) noexcept -> std::optional<Timestamp> {
    const auto [days, nanoseconds] = splitAmount(ticks);
    auto overflow = false;
    const auto result = epoch(epochValue).shifted(days, nanoseconds, false, overflow);
    if (overflow) {
        return std::nullopt;
    }
    return result;
}

template <impl::DateTimeTickUnit T>
auto Timestamp::fromTicksOrThrow(T ticks, TimeEpoch epochValue) -> Timestamp {
    const auto result = fromTicks(ticks, epochValue);
    if (!result) {
        throw err::OutOfRangeError{"Epoch ticks exceed the timestamp calendar range"_el};
    }
    return *result;
}

}
