// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../block/Size.hpp"

namespace erbsland::cterm::impl {

/// The default width for terminal screens and plain-text document layout.
inline constexpr auto cDefaultTerminalWidth = 120;
/// The initial terminal size before a size is supplied or detected.
inline constexpr auto cDefaultTerminalSize = block::Size{cDefaultTerminalWidth, 25};

}
