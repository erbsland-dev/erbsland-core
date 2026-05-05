// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U32StringTraits.hpp"

#include <string_view>

namespace erbsland::text::impl {

/// String data for UTF-32 encoded strings.
/// We allocate one additional code unit for the null terminator and always ensure it is set.
using U32StringData = U32StringTraits::StorageData;

/// A shared pointer to UTF-32 string data.
using U32StringDataPtr = U32StringTraits::StoragePtr;

/// Create UTF-32 string data from a standard string view.
/// The UTF-32 encoding is not validated, but the null terminator is always set.
/// @param stdString The standard string view to convert.
/// @tested{U32StringDataTest}
[[nodiscard]] auto createU32StringData(std::u32string_view stdString) -> U32StringDataPtr;
/// Create uninitialized UTF-32 string data for a string of the given size and reserved capacity.
/// @param actualStringDataSize The actual string data size, without a terminating null byte.
/// @param reservedCapacity The reserved capacity, without a terminating null byte.
/// @tested{U32StringDataTest}
[[nodiscard]] auto createU32StringData(std::size_t actualStringDataSize, std::size_t reservedCapacity)
    -> U32StringDataPtr;
/// Create uninitialized UTF-32 string data for a string of the given size.
/// @param actualStringDataSize The actual string data size, without a terminating null byte.
/// @tested{U32StringDataTest}
[[nodiscard]] auto createU32StringData(std::size_t actualStringDataSize) -> U32StringDataPtr;

}
