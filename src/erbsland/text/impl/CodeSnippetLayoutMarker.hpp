// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../String.hpp"

#include "../../unit/ColumnRange.hpp"

namespace erbsland::text::impl {

/// Marker information supplied to the renderer-neutral code snippet layout.
/// @tested{CodeSnippetLayoutTest}
struct CodeSnippetLayoutMarker final {
    unit::ColumnRange range; ///< The logical source range.
    String label;            ///< Optional marker label.
};

}
