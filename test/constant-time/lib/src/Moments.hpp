// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace app::constant_time {

/// Numerically stable online moments for one sample population.
/// @tested{ConstantTimeStatisticsTest}
class Moments final {
public:
    /// Add one finite measurement.
    void add(double value) noexcept;
    /// Get the sample count.
    [[nodiscard]] auto count() const noexcept -> uint64_t { return _count; }
    /// Get the running mean.
    [[nodiscard]] auto mean() const noexcept -> double { return _mean; }
    /// Get unbiased sample variance, or zero with fewer than two samples.
    [[nodiscard]] auto variance() const noexcept -> double;

private:
    uint64_t _count{}; ///< Number of measurements.
    double _mean{};    ///< Running mean.
    double _m2{};      ///< Sum of squared deviations.
};

}
