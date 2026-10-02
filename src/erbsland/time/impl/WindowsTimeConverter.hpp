// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../Timestamp.hpp"

#include "../../core/impl/WindowsApi.hpp"

#include <optional>

/// Conversion tools for native Windows FILETIME values.
namespace erbsland::time::impl::windows_time_converter {

/// Convert native FILETIME to a compact UTC instant.
/// @param fileTime The native value.
/// @return The timestamp, or invalid outside the calendar range.
/// @tested{WindowsTimeConverterTest}
[[nodiscard]] auto fromFileTime(const FILETIME &fileTime) noexcept -> Timestamp;
/// Convert a UTC timestamp, truncating fractions below 100 nanoseconds.
/// @param timestamp The timestamp.
/// @return The native value, or no value if it cannot be represented.
/// @tested{WindowsTimeConverterTest}
[[nodiscard]] auto toFileTime(const Timestamp &timestamp) noexcept -> std::optional<FILETIME>;

}
