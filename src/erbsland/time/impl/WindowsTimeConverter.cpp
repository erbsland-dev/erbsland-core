// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "WindowsTimeConverter.hpp"

#include <limits>

namespace erbsland::time::impl {

namespace {
constexpr auto cWindowsFileTimeTicksPerSecond = 10'000'000ULL;
constexpr auto cWindowsFileTimeTickNanoseconds = 100LL;
}

auto WindowsTimeConverter::epochSecondsDelta() noexcept -> Seconds {
    static const auto result = DateTime{Date::fromYearMonthDay(1601, 1, 1), Time{}}.toSecondsSinceEpoch();
    return result;
}

auto WindowsTimeConverter::fromFileTimeTicks(const std::uint64_t ticks) noexcept -> DateTime {
    const auto seconds = ticks / cWindowsFileTimeTicksPerSecond;
    const auto windowsEpoch = epochSecondsDelta().toRawValue();
    if (seconds > static_cast<std::uint64_t>(std::numeric_limits<int64_t>::max() - windowsEpoch)) {
        return {};
    }
    const auto fractionTicks = ticks % cWindowsFileTimeTicksPerSecond;
    const auto fractions = Nanoseconds{static_cast<int64_t>(fractionTicks * cWindowsFileTimeTickNanoseconds)};
    return DateTime::fromSecondsSinceEpoch(Seconds{windowsEpoch + static_cast<int64_t>(seconds)}, fractions);
}

auto WindowsTimeConverter::toFileTimeTicks(const DateTime &dateTime) noexcept -> std::optional<std::uint64_t> {
    if (!dateTime.isValid()) {
        return std::nullopt;
    }
    const auto secondsSinceEpoch = dateTime.toSecondsSinceEpoch();
    const auto windowsEpoch = epochSecondsDelta();
    if (secondsSinceEpoch < windowsEpoch) {
        return std::nullopt;
    }
    const auto seconds = secondsSinceEpoch - windowsEpoch;
    const auto secondsRaw = seconds.toRawValue();
    const auto secondsUnsigned = static_cast<std::uint64_t>(secondsRaw);
    if (secondsUnsigned > std::numeric_limits<std::uint64_t>::max() / cWindowsFileTimeTicksPerSecond) {
        return std::nullopt;
    }
    const auto secondTicks = secondsUnsigned * cWindowsFileTimeTicksPerSecond;
    const auto fractionTicks =
        static_cast<std::uint64_t>(dateTime.nanosecondFraction().toRawValue() / cWindowsFileTimeTickNanoseconds);
    return secondTicks + fractionTicks;
}

#ifdef ERBSLAND_OS_WINDOWS
auto WindowsTimeConverter::fromFileTime(const FILETIME &fileTime) noexcept -> DateTime {
    const auto high = static_cast<std::uint64_t>(fileTime.dwHighDateTime);
    const auto low = static_cast<std::uint64_t>(fileTime.dwLowDateTime);
    return fromFileTimeTicks((high << 32U) | low);
}

auto WindowsTimeConverter::toFileTime(const DateTime &dateTime) noexcept -> std::optional<FILETIME> {
    const auto ticks = toFileTimeTicks(dateTime);
    if (!ticks.has_value()) {
        return std::nullopt;
    }
    auto result = FILETIME{};
    result.dwLowDateTime = static_cast<DWORD>(*ticks & 0xffffffffULL);
    result.dwHighDateTime = static_cast<DWORD>(*ticks >> 32U);
    return result;
}
#endif

}
