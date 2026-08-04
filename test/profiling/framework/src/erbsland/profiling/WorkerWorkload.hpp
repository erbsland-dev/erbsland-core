// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Definitions.hpp"
#include "WorkerExecutionContext_fwd.hpp"
#include "WorkerMeasurement.hpp"
#include "WorkerWorkload_fwd.hpp"

namespace erbsland::profiling {

/// Independent mutable workload state for one worker.
/// @tested{WorkloadRunnerTest}
class WorkerWorkload {
public:
    // defaults
    virtual ~WorkerWorkload() = default;

public:
    /// Execute a calibrated operation batch.
    /// @param context Operations, deterministic seed and cancellation state.
    /// @return Workload measurements excluding framework wall timing.
    [[nodiscard]] virtual auto execute(const WorkerExecutionContext &context) -> WorkerMeasurement = 0;
};

}
