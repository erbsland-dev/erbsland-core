// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../BlockStyle.hpp"

#include "../../block/Position.hpp"

namespace erbsland::cterm::impl {

/// A saved emulation cursor and presentation state.
/// @tested{TerminalEmulationTest}
struct EmulationCursorState {
    block::Position position{};            ///< Saved logical position.
    BlockStyle style{BlockStyle::reset()}; ///< Saved presentation.
    bool wrapPending{false};               ///< Delayed right-edge wrap.
};

}
