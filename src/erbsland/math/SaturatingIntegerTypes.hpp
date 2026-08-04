// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "SaturatingInteger_fwd.hpp"

#include <concepts>
#include <type_traits>

namespace erbsland::math {

/// Test if a type is `SaturatingInteger`.
template <typename T>
struct IsSaturatingInteger : std::false_type {};

/// Specialization that identifies saturating integer values.
template <NativeInteger T>
struct IsSaturatingInteger<SaturatingInteger<T>> : std::true_type {};

// A `SaturatingInteger` type.
template <typename T>
concept SaturatingIntegerType = IsSaturatingInteger<std::remove_cvref_t<T>>::value;

}
