// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../core/Definitions.hpp"

#ifdef ERBSLAND_OS_WINDOWS
#include "../../core/impl/WindowsApi.hpp"
#endif

#include "../DateTime.hpp"

#include <cstdint>
#include <optional>

namespace erbsland::time::impl {

/// Conversion tools for Windows FILETIME values.
/// @tested{TimeConverterTest}
class WindowsTimeConverter final {
public:
    WindowsTimeConverter() = delete;

public:
    /// Convert Windows FILETIME 100-nanosecond ticks since 1601-01-01 UTC to a date/time.
    [[nodiscard]] static auto fromFileTimeTicks(std::uint64_t ticks) noexcept -> DateTime;
    /// Convert a date/time to Windows FILETIME 100-nanosecond ticks since 1601-01-01 UTC.
    [[nodiscard]] static auto toFileTimeTicks(const DateTime &dateTime) noexcept -> std::optional<std::uint64_t>;
#ifdef ERBSLAND_OS_WINDOWS
    /// Convert a native FILETIME value to a date/time.
    [[nodiscard]] static auto fromFileTime(const FILETIME &fileTime) noexcept -> DateTime;
    /// Convert a date/time to a native FILETIME value.
    [[nodiscard]] static auto toFileTime(const DateTime &dateTime) noexcept -> std::optional<FILETIME>;
#endif

private:
    [[nodiscard]] static auto epochSecondsDelta() noexcept -> Seconds;
};

}
