// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../core/Definitions.hpp"

#include <concepts>
#include <cstdint>
#include <type_traits>

namespace erbsland::math {

// A native integer type supported by the math helpers.
template <typename T>
concept NativeInteger = std::integral<T> && std::same_as<T, std::remove_cv_t<T>> && !std::same_as<bool, T>;

// Check if both types are supported native integer types.
template <typename tFirst, typename tSecond>
concept NativeIntegerPair = NativeInteger<tFirst> && NativeInteger<tSecond>;

}
