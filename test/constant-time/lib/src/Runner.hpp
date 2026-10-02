// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Result.hpp"
#include "RunOptions.hpp"
#include "TestCase.hpp"

#include <erbsland/core/Definitions.hpp>
#include <erbsland/MakeOneNamespace.hpp>
#include <erbsland/time/TimePoint.hpp>

#include <atomic>
#include <functional>

namespace app::constant_time {

/// Single-threaded, calibrated dudect-style measurement runner.
/// @tested{ConstantTimeRunnerTest}
class Runner final {
public:
    /// Create a runner with an injectable monotonic clock and optional interruption flag.
    /// @param clock Monotonic clock used for all budgets and samples.
    /// @param interrupted Cooperative interruption flag observed outside measured execution.
    explicit Runner(
        std::function<el::TimePoint()> clock = el::TimePoint::now, const std::atomic<bool> *interrupted = nullptr) :
        _clock{std::move(clock)}, _interrupted{interrupted} {}
    /// Execute one experiment within its budget.
    /// @param test Prepared-operation interface.
    /// @param options Wall-time and sample limits.
    /// @param progress Callback invoked only between measured batches.
    /// @return Final evidence and outcome.
    [[nodiscard]] auto run(TestCase &test, const RunOptions &options, std::function<void(const Result &)> progress = {})
        -> Result;

private:
    std::function<el::TimePoint()> _clock; ///< Injectable monotonic clock.
    const std::atomic<bool> *_interrupted; ///< Optional cooperative stop flag.
};
}
