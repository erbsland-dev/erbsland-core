// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

namespace erbsland::test {

/// Counts copies and moves made by a move-aware test value.
/// @notest{Used by tests to assert copy and move behavior.}
struct MoveAwareTestCounts final {
    int copies{0};
    int moves{0};
};

}
