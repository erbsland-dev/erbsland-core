// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../DateTime.hpp"
#include "../TimeAmounts.hpp"

#include <ctime>

namespace erbsland::time::impl {

/// Conversion tools for POSIX time values.
/// @tested{TimeConverterTest}
class PosixTimeConverter final {
public:
    PosixTimeConverter() = delete;

public:
    /// Convert a date/time to std::time_t.
    [[nodiscard]] static auto toTimeT(const DateTime &dateTime) noexcept -> std::time_t;
    /// Convert a std::time_t POSIX value to a date/time.
    [[nodiscard]] static auto fromTimeT(std::time_t posixTime) noexcept -> DateTime;
    /// Convert POSIX seconds and nanoseconds to a date/time.
    [[nodiscard]] static auto fromPosixTime(Seconds seconds, Nanoseconds fractions = Nanoseconds{}) noexcept
        -> DateTime;
    /// Convert a POSIX timespec to a date/time.
    [[nodiscard]] static auto fromTimespec(const timespec &timeSpec) noexcept -> DateTime;

private:
    [[nodiscard]] static auto epochSecondsDelta() noexcept -> Seconds;
};

}
