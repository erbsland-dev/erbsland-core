// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Fixture.hpp"
#include "PreparedScenario_fwd.hpp"

#include "../ProfileTypes.hpp"

namespace app::stream::impl {

/// Internal PreparedScenario data for stream profiling.
/// @notest{Covered by stream profiler CTest entries.}
struct PreparedScenario {
    Scenario scenario;                          ///< Expanded scenario.
    std::vector<std::vector<Fixture>> fixtures; ///< Fixtures grouped by worker.
    el::TempDirectoryPtr workspace;             ///< Temporary workspace.
};

}
