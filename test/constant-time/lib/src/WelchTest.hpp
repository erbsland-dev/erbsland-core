// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Moments.hpp"

#include <array>

namespace app::constant_time {

/// Welch comparison of two populations.
/// @tested{ConstantTimeStatisticsTest}
class WelchTest final {
public:
    /// Add one measurement to the selected population.
    void add(double value, bool population) noexcept;
    /// Get the selected population's moments.
    [[nodiscard]] auto population(bool value) const noexcept -> const Moments & { return _populations[value]; }
    /// Test whether both populations meet the minimum count and have usable variance.
    [[nodiscard]] auto isEligible(uint64_t minimum = 10000) const noexcept -> bool;
    /// Calculate the signed statistic, handling zero variance explicitly.
    [[nodiscard]] auto statistic() const noexcept -> double;

private:
    std::array<Moments, 2> _populations; ///< Independent sample populations.
};

}
