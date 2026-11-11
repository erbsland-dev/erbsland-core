// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <DemoCommon.hpp>
#include <erbsland/EventLoop.hpp>

#include <cstddef>

namespace demo {

/// Aggregate three independent readings with at most two requests outstanding.
/// @notest{Compiled demo verifies the result count and sum.}
class ReadingBatch {
public:
    /// Construct a batch that remains alive until all its queued callbacks finish.
    /// @param loop The owner loop, stopped at completion.
    explicit ReadingBatch(el::EventLoopPtr loop);

public: // operations
    /// Start the batch once on its owner loop.
    void start();

private:
    void launchAvailable();
    void receive(std::size_t index, int value);

private:
    el::EventLoopPtr _loop;     ///< Loop that owns all batch state.
    std::size_t _next = 0;      ///< Next request to launch.
    std::size_t _inFlight = 0;  ///< Requests without a result yet.
    std::size_t _completed = 0; ///< Results received so far.
    int _sum = 0;               ///< Aggregated observation count.
};

}
