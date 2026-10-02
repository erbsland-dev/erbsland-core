// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ElapsedTimerDemos.hpp"

#include <erbsland/time/ElapsedTimer.hpp>
#include <erbsland/time/Literals.hpp>

#include <algorithm>
#include <cstddef>
#include <vector>

namespace demo {

/// Process finite input in batches while checking a monotonic time budget.
/// Checking elapsed() between batches lets a caller retain its progress and continue later.
/// A time-budget check cannot interrupt the batch already in progress.
/// @notest{Compiled and executed documentation demo.}
void budget() {
    using namespace el::time::literals;
    const auto detector = "Algılayıcı"_el;
    const auto samples = std::vector<double>(1'000'000, 12.0);
    const auto timeBudget = el::TimeDelta{2_ms};
    constexpr auto cBatchSize = std::size_t{256};
    auto processed = std::size_t{0};
    auto total = 0.0;
    const auto timer = el::ElapsedTimer{};

    // Keep an input boundary as well as a time boundary, and do useful work in each batch.
    while (processed < samples.size() && timer.elapsed() < timeBudget) {
        const auto end = std::min(processed + cBatchSize, samples.size());
        for (; processed < end; ++processed) {
            total += samples[processed] - 2.0;
        }
    }
    const auto elapsed = timer.elapsed();

    el::io::printLine(
        el::StringFormat{"{}: processed {} of {} samples; calibrated total {}"_el}.build(
            detector, processed, samples.size(), total));
    el::io::printLine(
        el::StringFormat{"Budget: {}; elapsed: {}; complete: {}"_el}.build(
            timeBudget.toString(), elapsed.toString(), processed == samples.size()));
}

}
