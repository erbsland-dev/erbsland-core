// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "MetricDefinition_fwd.hpp"

#include <erbsland/all.hpp>

namespace erbsland::profiling {

/// A numeric workload metric and its reporting behavior.
/// @tested{ProfilingDefinitionTest}
struct MetricDefinition {
    String id;         ///< Stable metric identifier.
    String unit{};     ///< Output unit or an empty string.
    bool reportRate{}; ///< Report this metric per second.
};

}
