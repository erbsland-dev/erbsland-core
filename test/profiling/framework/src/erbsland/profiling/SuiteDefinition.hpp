// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Scenario.hpp"
#include "SuiteDefinition_fwd.hpp"

#include <erbsland/all.hpp>

namespace erbsland::profiling {

/// A named built-in scenario suite.
/// @tested{ProfilingDefinitionTest}
struct SuiteDefinition {
    String id;                ///< Stable suite identifier.
    List<Scenario> scenarios; ///< Concrete suite scenarios.
};

}
