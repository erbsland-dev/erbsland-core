// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Evidence.hpp"
#include "Outcome.hpp"
#include "TestMetadata.hpp"

#include <erbsland/core/Definitions.hpp>
#include <erbsland/MakeOneNamespace.hpp>
#include <erbsland/text/String.hpp>
#include <erbsland/time/TimeDelta.hpp>

namespace app::constant_time {

/// Completed measurement state with its own plain-record representation.
/// @tested{ConstantTimeRunnerTest}
class Result final {
    friend class Runner;

public:
    /// Create an experiment result, initially with insufficient evidence.
    /// @param metadata Exact scenario configuration.
    /// @param outcome Initial bounded-run outcome.
    /// @param seed Actual fixture seed.
    explicit Result(TestMetadata metadata = {}, Outcome outcome = Outcome::Insufficient, uint64_t seed = 0) :
        _metadata{std::move(metadata)}, _outcome{outcome}, _seed{seed} {}
    /// Get the measured scenario description.
    [[nodiscard]] auto metadata() const noexcept -> const TestMetadata & { return _metadata; }
    /// Get the bounded-run outcome.
    [[nodiscard]] auto outcome() const noexcept -> Outcome { return _outcome; }
    /// Get the actual fixture seed.
    [[nodiscard]] auto seed() const noexcept -> uint64_t { return _seed; }
    /// Get the operation count per sample.
    [[nodiscard]] auto repetitions() const noexcept -> uint64_t { return _repetitions; }
    /// Get accepted measurements from one population.
    /// @param population The population to inspect.
    /// @return Accepted measurements.
    [[nodiscard]] auto samples(bool population) const noexcept -> uint64_t { return _samples[population]; }
    /// Get rejected unusable measurements.
    [[nodiscard]] auto rejected() const noexcept -> uint64_t { return _rejected; }
    /// Get the complete setup and measurement elapsed time.
    [[nodiscard]] auto elapsed() const noexcept -> el::TimeDelta { return _elapsed; }
    /// Get the largest eligible statistical channel.
    [[nodiscard]] auto evidence() const noexcept -> const Evidence & { return _evidence; }
    /// Get the actionable outcome detail.
    [[nodiscard]] auto detail() const noexcept -> const el::String & { return _detail; }
    /// Replace the actionable detail after a controlled or failed run.
    /// @param detail Plain-text explanation.
    void setDetail(el::String detail) { _detail = std::move(detail); }
    /// Get the stable textual outcome name.
    [[nodiscard]] auto outcomeName() const -> el::String;
    /// Format a complete redirected-output-safe record.
    [[nodiscard]] auto toString() const -> el::String;

private:
    TestMetadata _metadata{};                ///< Exact measured operation and classes.
    Outcome _outcome{Outcome::Insufficient}; ///< Bounded-run outcome.
    uint64_t _seed{};                        ///< Actual fixture seed.
    uint64_t _repetitions{1};                ///< Operations per measured sample.
    uint64_t _samples[2]{};                  ///< Accepted measurements by population.
    uint64_t _rejected{};                    ///< Unusable timer observations.
    el::TimeDelta _elapsed{};                ///< Entire preparation/calibration/measurement time.
    Evidence _evidence{};                    ///< Largest eligible statistic.
    el::String _detail{};                    ///< Actionable result detail.
};

}
