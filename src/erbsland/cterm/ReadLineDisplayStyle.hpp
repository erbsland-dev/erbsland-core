// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../core/Definitions.hpp"

#include <cstdint>

namespace erbsland::cterm {

/// The layout used to display an interactive read-line area.
enum class ReadLineDisplayStyle : std::uint8_t {
    Compact,         ///< Title and input rows without trailing spacing or a frame.
    HorizontalSpace, ///< Compact layout followed by one empty row.
    HorizontalFrame, ///< Title in a top horizontal border and a bottom horizontal border.
    Frame,           ///< A complete frame around the title and input rows.
};

}
