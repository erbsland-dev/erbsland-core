// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../Block.hpp"

#include "../../unit/CpIndex.hpp"

#include <cstddef>
#include <vector>

namespace erbsland::cterm::impl {

/// One rendered character cell in a read-line layout.
/// @notest{Covered through ReadLineBase rendering tests.}
struct ReadLineCell final {
    int column{0};
    unit::CpIndex startIndex;
    unit::CpIndex endIndex;
    Block block;
};

/// A cursor boundary in a read-line layout row.
/// @notest{Covered through ReadLineBase rendering tests.}
struct ReadLineBoundary final {
    int column{0};
    unit::CpIndex index;
};

/// The rendered cells and cursor boundaries for one row.
/// @notest{Covered through ReadLineBase rendering tests.}
struct ReadLineLayoutRow final {
    std::vector<ReadLineCell> cells;
    std::vector<ReadLineBoundary> boundaries;
};

/// The complete rendered layout of a read-line interaction.
/// @notest{Covered through ReadLineBase rendering tests.}
struct ReadLineLayout final {
    /// A row-and-column cursor position in the layout.
    struct CursorPosition final {
        std::size_t row{0U};
        int column{0};
    };

    int terminalWidth{1};
    int contentLeft{0};
    int contentRight{1};
    int promptWidth{0};
    int textColumn{0};
    int editWidth{1};
    std::vector<ReadLineLayoutRow> rows;
    std::vector<CursorPosition> cursorPositions;
};

}
