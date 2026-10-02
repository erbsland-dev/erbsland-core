// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Evidence.hpp"
#include "WelchTest.hpp"

#include <erbsland/core/Definitions.hpp>
#include <erbsland/MakeOneNamespace.hpp>
#include <erbsland/util/List.hpp>

#include <array>

namespace app::constant_time {

/// Dudect's uncropped, 100 cropped, and centered-square statistical channels.
/// @tested{ConstantTimeStatisticsTest}
class Statistics final {
public:
    /// Establish class-independent crop thresholds using a discarded pilot batch.
    /// @param pilot Positive elapsed measurements from both populations.
    void initialize(el::List<double> pilot);
    /// Add one elapsed measurement, updating all applicable channels.
    void add(double nanoseconds, bool population);
    /// Get the strongest eligible statistic.
    [[nodiscard]] auto evidence() const -> Evidence;
    /// Get raw population statistics.
    [[nodiscard]] auto raw() const noexcept -> const WelchTest & { return _tests[0]; }
    /// Test whether raw and second-order channels have enough usable measurements.
    [[nodiscard]] auto hasEnoughEvidence() const noexcept -> bool;
    /// Get a crop threshold for deterministic verification.
    [[nodiscard]] auto threshold(std::size_t index) const -> double { return _thresholds.at(index); }

private:
    std::array<WelchTest, 102> _tests;     ///< Raw, cropped, and second-order comparisons.
    std::array<double, 100> _thresholds{}; ///< Pilot-derived class-independent thresholds.
};
}
