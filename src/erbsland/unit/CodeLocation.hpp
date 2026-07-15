// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ColumnIndex.hpp"
#include "CpIndex.hpp"
#include "LineIndex.hpp"

namespace erbsland::unit {

/// A source code location with optional line, column and code-point position.
/// @tested{IntegerUnitTest}
struct CodeLocation final {
    LineIndex line{LineIndex::noIndex()};       ///< The zero-based line index, if known.
    ColumnIndex column{ColumnIndex::noIndex()}; ///< The zero-based column index, if known.
    CpIndex position{CpIndex::noIndex()};       ///< The zero-based code-point position, if known.
};

}
