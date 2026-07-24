// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "AxisValue_fwd.hpp"

#include <erbsland/all.hpp>

namespace erbsland::profiling {

/// One stable value registered for a categorical axis.
/// @tested{ProfilingDefinitionTest}
struct AxisValue {
    String id;            ///< Stable configuration and output identifier.
    String description{}; ///< Human-readable description.
};

}
