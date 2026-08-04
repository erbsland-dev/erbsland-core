// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Configuration.hpp"

namespace app::byte {

/// Execute expanded byte-type profiling scenarios.
/// @notest{Covered by byte profiler smoke and benchmark CTest entries.}
class WorkloadRunner final {
public:
    /// Create a workload runner.
    /// @param configuration Effective profiler configuration.
    explicit WorkloadRunner(Configuration configuration);

public:
    /// Print expanded scenarios without executing workloads.
    void printDryRun() const;
    /// Print the use-case and API coverage registry.
    void printCoverage() const;
    /// Run all selected scenarios.
    /// @return A successful exit code.
    [[nodiscard]] auto run() -> el::ExitCode;

private:
    Configuration _configuration;
};

}
