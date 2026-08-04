// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "WindowsTimeConverter.hpp"

namespace erbsland::time::impl::windows_time_converter {

auto fromFileTime(const FILETIME &fileTime) noexcept -> DateTime {
    const auto high = static_cast<std::uint64_t>(fileTime.dwHighDateTime);
    const auto low = static_cast<std::uint64_t>(fileTime.dwLowDateTime);
    const auto ticks = (high << 32U) | low;
    const auto seconds = Seconds{static_cast<int64_t>(ticks / 10'000'000ULL)};
    const auto fractions = Nanoseconds{static_cast<int64_t>((ticks % 10'000'000ULL) * 100ULL)};
    return DateTime::fromTicks(seconds, fractions, TimeEpoch::Windows).value_or(DateTime{});
}

auto toFileTime(const DateTime &dateTime) noexcept -> std::optional<FILETIME> {
    const auto secondsAndFractions = dateTime.toSecondsAndFractions(TimeEpoch::Windows);
    if (!secondsAndFractions.has_value()) {
        return std::nullopt;
    }
    const auto &[seconds, fractions] = *secondsAndFractions;
    constexpr auto cFileTimeTicksPerSecond = 10'000'000LL;
    constexpr auto cFileTimeTickNanoseconds = 100LL;
    if (seconds.toValue().wouldMultiplySaturate(cFileTimeTicksPerSecond)) {
        return std::nullopt;
    }
    const auto fractionTicks = fractions.toValue().divided(cFileTimeTickNanoseconds);
    const auto ticks = seconds.toValue().multiplied(cFileTimeTicksPerSecond).added(fractionTicks);
    if (ticks.isNegative()) {
        return std::nullopt;
    }
    auto result = FILETIME{};
    const auto value = static_cast<std::uint64_t>(ticks.toRawValue());
    result.dwLowDateTime = static_cast<DWORD>(value & 0xffffffffULL);
    result.dwHighDateTime = static_cast<DWORD>(value >> 32U);
    return result;
}

}
