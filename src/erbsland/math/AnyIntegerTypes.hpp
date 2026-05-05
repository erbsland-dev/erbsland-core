// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "IntegerTypes.hpp"
#include "SaturatingIntegerTypes.hpp"

namespace erbsland::math {

/// A type that can be used as an integer operand.
template <typename T>
concept AnyIntegerType = NativeInteger<std::remove_cvref_t<T>> || SaturatingIntegerType<T>;

/// Check if both types can be used as integer operands.
template <typename tFirst, typename tSecond>
concept AnyIntegerPair = AnyIntegerType<tFirst> && AnyIntegerType<tSecond>;

}
