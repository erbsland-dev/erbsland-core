// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CaptureGroupManager.hpp"
#include "Counter.hpp"
#include "Program.hpp"

#include "../Limits.hpp"

#include "../../InputPosition.hpp"

#include <algorithm>
#include <cstddef>
#include <vector>

namespace erbsland::re::impl {

/// A thread in the engine.
class EngineThread {
public:
    /// Create a new thread for the given program position.
    explicit constexpr EngineThread(const ProgramCounter programCounter) noexcept : programCounter{programCounter} {}

    // defaults
    EngineThread() = default;
    EngineThread(const EngineThread &) = default;
    EngineThread(EngineThread &&) = default;
    auto operator=(const EngineThread &) -> EngineThread & = default;
    auto operator=(EngineThread &&) -> EngineThread & = default;
    ~EngineThread() = default;

public:
    /// Compare engine threads for equality.
    auto operator==(const EngineThread &other) const noexcept -> bool {
        return programCounter == other.programCounter && sequenceCounter == other.sequenceCounter &&
            counter == other.counter;
    }
    /// Compare engine threads for inequality.
    auto operator!=(const EngineThread &other) const noexcept -> bool { return !(*this == other); }

    /// Test if two threads have the same execution state, comparing only counters used by the program.
    [[nodiscard]] auto hasSameExecutionState(const EngineThread &other, const std::size_t counterCount) const noexcept
        -> bool {
        return programCounter == other.programCounter && sequenceCounter == other.sequenceCounter &&
            std::equal(
                counter.begin(), counter.begin() + static_cast<std::ptrdiff_t>(counterCount), other.counter.begin());
    }

public:
    ProgramCounter programCounter{0};                               ///< The program counter.
    CounterType sequenceCounter{0};                                 ///< A dedicated counter for "sequence" operations.
    std::array<CounterType, limits::maximumCounterCount> counter{}; ///< The counters for the thread.
    CaptureGroupSetReference captureGroupSet{0};                    ///< The current capture set.
    InputPosition matchStart{};                                     ///< Direct whole-match start without captures.
};

using EngineThreadList = std::vector<EngineThread>;

}
