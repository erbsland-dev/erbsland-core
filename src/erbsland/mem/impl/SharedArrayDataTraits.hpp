// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <concepts>
#include <cstdint>
#include <type_traits>

namespace erbsland::mem::impl {

/// The supported size types for shared array data.
/// Only 32-bit and 64-bit unsigned sizes are allowed to keep storage headers predictable and avoid accidental use with
/// small or signed types.
///
/// @warning This is an advanced data type, meant for people extending the library.
/// Do not use it unless you understand the implications and have a specific need.
///
/// @tparam tSizeType The size type to test.
template <typename tSizeType>
concept SharedArrayDataSizeType = std::same_as<tSizeType, uint32_t> || std::same_as<tSizeType, uint64_t>;

}
