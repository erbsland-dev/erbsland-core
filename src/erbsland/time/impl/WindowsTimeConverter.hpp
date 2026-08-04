// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../DateTime.hpp"

#include "../../core/impl/WindowsApi.hpp"

#include <optional>

/// Conversion tools for native Windows FILETIME values.
namespace erbsland::time::impl::windows_time_converter {

/// Convert a native FILETIME value to a date/time.
[[nodiscard]] auto fromFileTime(const FILETIME &fileTime) noexcept -> DateTime;
/// Convert a date/time to a native FILETIME value, truncating fractional precision below 100 nanoseconds.
[[nodiscard]] auto toFileTime(const DateTime &dateTime) noexcept -> std::optional<FILETIME>;

}
