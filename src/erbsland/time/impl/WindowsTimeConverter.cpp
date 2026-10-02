// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "WindowsTimeConverter.hpp"

namespace erbsland::time::impl::windows_time_converter {

auto fromFileTime(const FILETIME &fileTime) noexcept -> Timestamp {
    const auto ticks = (static_cast<uint64_t>(fileTime.dwHighDateTime) << 32U) | fileTime.dwLowDateTime;
    return Timestamp::fromTicks(
        Seconds{static_cast<int64_t>(ticks / 10'000'000ULL)},
        Nanoseconds{static_cast<int64_t>((ticks % 10'000'000ULL) * 100ULL)},
        TimeEpoch::Windows)
        .value_or(Timestamp{});
}

auto toFileTime(const Timestamp &timestamp) noexcept -> std::optional<FILETIME> {
    const auto parts = timestamp.toSecondsAndFractions(TimeEpoch::Windows);
    if (!parts || parts->first.isNegative()) {
        return std::nullopt;
    }
    // Calendar bounds keep the full unsigned FILETIME calculation within uint64_t.
    const auto ticks = static_cast<uint64_t>(parts->first.toRawValue()) * 10'000'000ULL +
        static_cast<uint64_t>(parts->second.toRawValue()) / 100ULL;
    return FILETIME{static_cast<DWORD>(ticks), static_cast<DWORD>(ticks >> 32U)};
}

}
