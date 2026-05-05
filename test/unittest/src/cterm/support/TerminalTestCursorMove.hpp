// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TestHelper.hpp"

struct TerminalTestCursorMove final {
    bgeo::BlockPosition pos{};
    MoveMode mode = MoveMode::Absolute;

    auto operator==(const TerminalTestCursorMove &) const noexcept -> bool = default;
};
