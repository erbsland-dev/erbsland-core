// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ProfilingConfiguration_fwd.hpp"
#include "RunConfiguration.hpp"
#include "Scenario.hpp"

#include <erbsland/all.hpp>

namespace erbsland::profiling {

/// The validated and expanded common profiling configuration.
/// @tested{ConfigurationLoaderTest WorkloadRunnerTest}
struct ProfilingConfiguration {
    RunConfiguration run;     ///< Common run settings.
    List<Scenario> scenarios; ///< Expanded scenarios.
    ByteBlock digest;         ///< Effective configuration digest.
};

}
