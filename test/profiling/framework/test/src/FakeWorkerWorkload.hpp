// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "FakeWorkerWorkload_fwd.hpp"

#include <erbsland/profiling/WorkerWorkload.hpp>

#include <atomic>
#include <memory>

/// A deterministic profiling worker used by framework tests.
/// @tested{WorkloadRunnerTest}
class FakeWorkerWorkload final : public erbsland::profiling::WorkerWorkload {
public:
    /// Create a deterministic worker workload with a shared execution counter.
    FakeWorkerWorkload(std::shared_ptr<std::atomic<std::uint64_t>> executions, bool fail);

public: // implement WorkerWorkload
    [[nodiscard]] auto execute(const erbsland::profiling::WorkerExecutionContext &context)
        -> erbsland::profiling::WorkerMeasurement override;

private:
    std::shared_ptr<std::atomic<std::uint64_t>> _executions;
    bool _fail{};
};
