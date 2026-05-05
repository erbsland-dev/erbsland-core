// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../math/AnyIntegerTypes.hpp"
#include "../../math/IntegerTraits.hpp"

#include <concepts>
#include <ratio>

namespace erbsland::unit::impl {

// A valid value type for a signed integer amount.
template <typename T>
concept ValidIntegerAmountValue = math::SaturatingIntegerType<T> && std::signed_integral<math::NativeIntegerOfT<T>>;

// A valid positive ratio.
template <typename T>
concept PositiveRatio = requires { typename std::ratio<T::num, T::den>; } && (T::num > 0) && (T::den > 0);

// A ratio that can be represented by simple integer scaling.
template <typename T>
concept SimpleIntegerAmountRatio = PositiveRatio<T> && (T::num == 1 || T::den == 1);

// A valid unit ratio for a signed integer amount.
template <typename T>
concept ValidIntegerAmountRatio = SimpleIntegerAmountRatio<T>;

// A type that follows the `IntegerAmount` interface.
template <typename T>
concept IntegerAmountType = requires {
    typename T::Unit;
    typename T::Ratio;
    typename T::Value;
    typename T::NativeValue;
    T::cIsIntegerAmount;
} && T::cIsIntegerAmount && ValidIntegerAmountValue<typename T::Value> && ValidIntegerAmountRatio<typename T::Ratio>;

// A signed integer operand for `IntegerAmount`.
template <typename T>
concept SignedIntegerAmountOperand = math::AnyIntegerType<T> && std::signed_integral<math::NativeIntegerOfT<T>>;

// A compatible conversion target for an `IntegerAmount`.
template <typename T, typename tUnit>
concept CompatibleIntegerAmount = IntegerAmountType<T> && std::same_as<tUnit, typename T::Unit>;

}
