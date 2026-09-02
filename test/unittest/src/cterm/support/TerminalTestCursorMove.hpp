// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TestHelper.hpp"

/// Recorded cursor movement used for terminal assertions.
/// @notest{Test-only value object.}
struct TerminalTestCursorMove final {
    block::Position pos{};
    MoveMode mode = MoveMode::Absolute;

    auto operator==(const TerminalTestCursorMove &) const noexcept -> bool = default;
};
