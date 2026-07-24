// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "AxisSelection.hpp"
#include "ParameterSelection.hpp"
#include "Scenario_fwd.hpp"

#include <erbsland/all.hpp>

#include <cstdint>

namespace erbsland::profiling {

/// One fully expanded generic profiling scenario.
/// @tested{ConfigurationLoaderTest WorkloadRunnerTest}
struct Scenario {
    String id;                             ///< Stable expanded identifier.
    String group;                          ///< Human-readable scenario group.
    String functionality;                  ///< Registered functionality identifier.
    List<AxisSelection> axes{};            ///< Selected categorical axes.
    List<ParameterSelection> parameters{}; ///< Typed scenario parameters.
    std::uint32_t weight{1U};              ///< Profile-mode repetition weight.
};

}
