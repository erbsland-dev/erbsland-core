// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../text/String.hpp"

#include <vector>

namespace erbsland::options::impl {

/// A display row used by option renderers.
/// @tested{StandardOptionRendererTest TerminalOptionsRendererTest}
struct OptionDisplayRow final {
    text::String title;                    ///< The left column or item title.
    text::String description;              ///< The row description.
    std::vector<OptionDisplayRow> details; ///< Detail rows attached to this row.
};

}
