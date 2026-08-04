// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ProfileTypes.hpp"

#include <optional>

namespace app::string {

/// Optional command-line settings applied before scenario expansion.
/// @notest{Covered by string profiler CLI CTest entries.}
struct RunOverrides {
    std::optional<RunMode> mode;                          ///< Optional execution-mode override.
    std::optional<std::uint32_t> threadCount;             ///< Optional worker-count override.
    std::optional<SensitiveSelection> sensitiveSelection; ///< Optional U8 sensitivity override.
};

}
