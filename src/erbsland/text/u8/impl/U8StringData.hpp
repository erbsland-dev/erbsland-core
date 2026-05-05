// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U8StringTraits.hpp"

#include "../../impl/UnsafeU8StringAccess_fwd.hpp"

#include <string_view>

namespace erbsland::text::impl {

/// String data for UTF-8 encoded strings.
/// We allocate one additional byte for the null terminator and always ensure it is set.
/// Therefore, even strongly discouraged, Low-level implementations can safely access the data via `const char*`.
using U8StringData = U8StringTraits::StorageData;

/// A shared pointer to UTF-8 string data.
using U8StringDataPtr = U8StringTraits::StoragePtr;

/// Create UTF-8 string data from a standard string view.
/// The UTF-8 encoding is not validated, but the null terminator is always set.
/// @param stdString The standard string view to convert.
/// @tested{U8StringDataTest}
[[nodiscard]] auto createU8StringData(std::string_view stdString) -> U8StringDataPtr;
/// @overload
/// @tested{U8StringDataTest}
[[nodiscard]] auto createU8StringData(std::u8string_view stdString) -> U8StringDataPtr;
/// Create uninitialized UTF-8 string data for a string of the given size and reserved capacity.
/// @param actualStringDataSize The actual string data size, without a terminating null byte.
/// @param reservedCapacity The reserved capacity, without a terminating null byte.
/// @tested{U8StringDataTest}
[[nodiscard]] auto createU8StringData(std::size_t actualStringDataSize, std::size_t reservedCapacity)
    -> U8StringDataPtr;
/// Create uninitialized UTF-8 string data for a string of the given size.
/// @param actualStringDataSize The actual string data size, without a terminating null byte.
/// @tested{U8StringDataTest}
[[nodiscard]] auto createU8StringData(std::size_t actualStringDataSize) -> U8StringDataPtr;

}
