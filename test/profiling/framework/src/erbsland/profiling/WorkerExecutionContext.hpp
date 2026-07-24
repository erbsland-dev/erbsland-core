// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "WorkerExecutionContext_fwd.hpp"

#include <cstdint>
#include <stop_token>

namespace erbsland::profiling {

/// Immutable framework state passed to a worker execution.
/// @tested{WorkloadRunnerTest}
struct WorkerExecutionContext {
    std::uint64_t operations{}; ///< Requested logical operation count.
    std::uint64_t seed{};       ///< Deterministic worker/sample seed.
    std::stop_token stopToken;  ///< Cooperative cancellation token.
};

}
