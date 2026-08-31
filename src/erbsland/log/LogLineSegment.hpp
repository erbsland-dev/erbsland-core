// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "LogLinePart.hpp"

#include "../text/String.hpp"

namespace erbsland::log {

/// One semantic segment of a formatted line.
/// @tested{LogCoreTest}
struct LogLineSegment final {
    LogLinePart part{LogLinePart::Literal}; ///< Semantic role of this segment.
    text::String text;                      ///< Rendered segment text.
};

}
