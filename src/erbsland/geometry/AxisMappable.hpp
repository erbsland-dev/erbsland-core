// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Axis.hpp"
#include "Dimensionality.hpp"
#include "SignedAxis.hpp"

#include <concepts>
#include <type_traits>

namespace erbsland::geometry {

/// A geometry value that can be reconstructed from mapped axis components.
template <typename T>
concept AxisMappable =
    requires(const T value, const Axis axis, const SignedAxis signedAxis) {
        typename T::AxisComponent;
        requires std::same_as<std::remove_cv_t<decltype(T::cDimensionality)>, Dimensionality>;
        { value.component(axis) } -> std::same_as<typename T::AxisComponent>;
        { value.component(signedAxis) } -> std::same_as<typename T::AxisComponent>;
    } &&
    ((T::cDimensionality == Dimensionality::One && std::constructible_from<T, typename T::AxisComponent>) ||
        (T::cDimensionality == Dimensionality::Two &&
            std::constructible_from<T, typename T::AxisComponent, typename T::AxisComponent>) ||
        (T::cDimensionality == Dimensionality::Three &&
            std::constructible_from<
                T,
                typename T::AxisComponent,
                typename T::AxisComponent,
                typename T::AxisComponent>));

}
