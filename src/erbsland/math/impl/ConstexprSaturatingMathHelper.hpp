// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../IntegerTraits.hpp"

namespace erbsland::math::impl {

// Test if a bounded constexpr arithmetic helper can safely use sign/magnitude arithmetic for the given types.
// The helper intentionally supports mixed signedness, but only for same-size operands and result bounds. Same-size
// inputs let the implementation represent every possible operand magnitude in the unsigned counterpart of the result
// type, including signed minimum values and unsigned maximum values, without depending on a wider integer type.
template <typename tFirst, typename tSecond, typename tResult>
concept SameSizeBoundedIntegerOperation = NativeInteger<tFirst> && NativeInteger<tSecond> && NativeInteger<tResult> &&
    (sizeof(tFirst) == sizeof(tSecond)) && (sizeof(tFirst) == sizeof(tResult));

}
