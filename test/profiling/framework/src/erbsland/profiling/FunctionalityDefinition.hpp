// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "FunctionalityDefinition_fwd.hpp"
#include "Scenario_fwd.hpp"
#include "ScenarioWorkload_fwd.hpp"

#include <erbsland/all.hpp>

#include <functional>

namespace erbsland::profiling {

/// A registered measurable functionality.
/// @tested{ProfilingDefinitionTest}
struct FunctionalityDefinition {
    using Factory = std::function<ScenarioWorkloadPtr(const Scenario &)>; ///< Workload factory callback.

    String id;                                                            ///< Stable functionality identifier.
    String description{};                                                 ///< Human-readable coverage description.
    StringList api{};                                                     ///< Represented public API families.
    Factory factory{};                                                    ///< Scenario workload factory.
};

}
