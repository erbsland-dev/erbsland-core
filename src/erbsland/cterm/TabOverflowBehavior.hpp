// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::cterm {

/// The handling for tabs whose configured tab stop does not advance the current line.
///
/// This mode is used when a left-aligned paragraph encounters a tab stop that is less than or equal to the current
/// column, or when there is no further configured tab stop.
enum class TabOverflowBehavior : uint8_t {
    /// Replace the tab with a single space character.
    AddSpace,
    /// End the current physical line and continue after the tab on the next wrapped line.
    LineBreak,
};

}
