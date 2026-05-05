// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U16StringTraits.hpp"

#include "../../impl/UnsafeU16StringAccess_fwd.hpp"

#include <string_view>

namespace erbsland::text::impl {

/// String data for UTF-16 encoded strings.
/// We allocate one additional code unit for the null terminator and always ensure it is set.
using U16StringData = U16StringTraits::StorageData;

/// A shared pointer to UTF-16 string data.
using U16StringDataPtr = U16StringTraits::StoragePtr;

/// Create UTF-16 string data from a standard string view.
/// The UTF-16 encoding is not validated, but the null terminator is always set.
/// @param stdString The standard string view to convert.
/// @tested{U16StringDataTest}
[[nodiscard]] auto createU16StringData(std::u16string_view stdString) -> U16StringDataPtr;
/// Create uninitialized UTF-16 string data for a string of the given size and reserved capacity.
/// @param actualStringDataSize The actual string data size, without a terminating null byte.
/// @param reservedCapacity The reserved capacity, without a terminating null byte.
/// @tested{U16StringDataTest}
[[nodiscard]] auto createU16StringData(std::size_t actualStringDataSize, std::size_t reservedCapacity)
    -> U16StringDataPtr;
/// Create uninitialized UTF-16 string data for a string of the given size.
/// @param actualStringDataSize The actual string data size, without a terminating null byte.
/// @tested{U16StringDataTest}
[[nodiscard]] auto createU16StringData(std::size_t actualStringDataSize) -> U16StringDataPtr;

}
