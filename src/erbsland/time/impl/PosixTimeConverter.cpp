// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PosixTimeConverter.hpp"

#include <cstdint>

namespace erbsland::time::impl {

namespace {
constexpr auto cNanosecondsPerSecond = 1'000'000'000LL;
}

auto PosixTimeConverter::epochSecondsDelta() noexcept -> Seconds {
    static const auto result = DateTime::posixEpoch().toSecondsSinceEpoch();
    return result;
}

auto PosixTimeConverter::toTimeT(const DateTime &dateTime) noexcept -> std::time_t {
    const auto secondTimeT = (dateTime.toSecondsSinceEpoch() - epochSecondsDelta()).toValue();
    // safe cast that saturate at the std::time_t value range.
    return static_cast<std::time_t>(secondTimeT.cast<std::time_t>().toRawValue());
}

auto PosixTimeConverter::fromTimeT(const std::time_t posixTime) noexcept -> DateTime {
    return fromPosixTime(Seconds{static_cast<int64_t>(posixTime)});
}

auto PosixTimeConverter::fromPosixTime(const Seconds seconds, const Nanoseconds fractions) noexcept -> DateTime {
    if (fractions.isNegative() || fractions >= Nanoseconds{cNanosecondsPerSecond}) {
        return {};
    }
    return DateTime::fromSecondsSinceEpoch(epochSecondsDelta() + seconds, fractions);
}

auto PosixTimeConverter::fromTimespec(const timespec &timeSpec) noexcept -> DateTime {
    return fromPosixTime(Seconds{static_cast<int64_t>(timeSpec.tv_sec)}, Nanoseconds{timeSpec.tv_nsec});
}

}
