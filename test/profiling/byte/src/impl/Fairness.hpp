// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Fairness_fwd.hpp"

namespace app::byte::impl {

/// Worker-fairness measurements for one byte-profiler sample.
/// @notest{Covered by byte profiler multi-worker CTest entries.}
struct Fairness {
    double minimum{};                ///< Minimum worker rate.
    double maximum{};                ///< Maximum worker rate.
    double coefficientOfVariation{}; ///< Coefficient of variation across workers.
};

}
