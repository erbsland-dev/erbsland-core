// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::text::render::impl {

/// Optional renderer work counters used by deterministic profiling.
/// @notest{Covered by the render profiler smoke and determinism tests.}
struct RenderCounters {
    uint64_t includeExecutions{};       ///< Executed include instructions.
    uint64_t blockExecutions{};         ///< Executed block implementations.
    uint64_t superExecutions{};         ///< Successful super dispatches.
    uint64_t capturedSuperExecutions{}; ///< Super dispatches rendered into temporary text.
};

}
