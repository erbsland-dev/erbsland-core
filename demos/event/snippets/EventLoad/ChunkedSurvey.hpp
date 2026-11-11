// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <DemoCommon.hpp>
#include <erbsland/EventLoop.hpp>

#include <cstddef>

namespace demo {

/// Aggregate a finite survey in small callbacks with one pending continuation.
class ChunkedSurvey final {
public:
    /// Retain the loop used to schedule each chunk.
    /// @param loop The open loop that owns the survey state.
    explicit ChunkedSurvey(el::EventLoopPtr loop);

public: // accessors
    /// Get the number of completed work units.
    [[nodiscard]] auto processedCount() const noexcept -> std::size_t { return _next; }
    /// Get the accumulated observation count.
    [[nodiscard]] auto total() const noexcept -> int { return _total; }

public: // operations
    /// Process at most two readings and queue a continuation when work remains.
    /// Call initially on the owner loop and retain this object until dispatch ends.
    void advance();

private:
    el::EventLoopPtr _loop; ///< Loop that owns and dispatches this survey.
    std::size_t _next = 0;  ///< Next reading to process, retained between callbacks.
    int _total = 0;         ///< Accumulated observations.
};

}
