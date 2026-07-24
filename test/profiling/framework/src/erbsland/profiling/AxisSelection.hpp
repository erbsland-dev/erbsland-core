// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "AxisSelection_fwd.hpp"

#include <erbsland/all.hpp>

namespace erbsland::profiling {

/// One selected categorical value in an expanded scenario.
/// @tested{ConfigurationLoaderTest}
struct AxisSelection {
    String axis;  ///< Axis identifier.
    String value; ///< Selected value identifier.
};

}
