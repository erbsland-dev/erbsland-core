// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U8StringData_fwd.hpp"

#include "../../../mem/SharedArrayData.hpp"

#include <string_view>

namespace erbsland::text::impl {

/// Create UTF-8 string data from a standard string view.
/// The UTF-8 encoding is not validated, but the null terminator is always set.
/// @param stdString The standard string view to convert.
[[nodiscard]] auto createU8StringData(std::string_view stdString) -> U8StringDataPtr;
/// @overload
[[nodiscard]] auto createU8StringData(std::u8string_view stdString) -> U8StringDataPtr;
/// Create uninitialized UTF-8 string data for a string of the given size and reserved capacity.
/// @param actualStringDataSize The actual string data size, without a terminating null byte.
/// @param reservedCapacity The reserved capacity, without a terminating null byte.
[[nodiscard]] auto createU8StringData(std::size_t actualStringDataSize, std::size_t reservedCapacity)
    -> U8StringDataPtr;
/// Create uninitialized UTF-8 string data for a string of the given size.
/// @param actualStringDataSize The actual string data size, without a terminating null byte.
[[nodiscard]] auto createU8StringData(std::size_t actualStringDataSize) -> U8StringDataPtr;

}
