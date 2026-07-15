// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../StringView.hpp"

#include "../../unit/ColumnRange.hpp"

namespace erbsland::text::impl {

/// Marker information supplied to the renderer-neutral code snippet layout.
struct CodeSnippetLayoutMarker final {
    unit::ColumnRange range; ///< The logical source range.
    StringView label;        ///< Optional marker label.
};

}
