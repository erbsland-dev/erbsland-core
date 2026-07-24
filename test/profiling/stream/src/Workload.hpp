// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ProfileTypes.hpp"

namespace app::stream {

/// Execute expanded file-stream profiling scenarios.
/// @notest{Covered by one-thread and four-thread profiler smoke tests.}
class WorkloadRunner final {
public:
    /// Create a workload runner.
    /// @param configuration Effective profiler configuration.
    explicit WorkloadRunner(Configuration configuration);

public:
    /// Print the expanded work without touching the filesystem.
    void printDryRun() const;
    /// Run all selected scenarios.
    /// @return A successful application exit code.
    [[nodiscard]] auto run() -> el::ExitCode;

private:
    Configuration _configuration;
};

}
