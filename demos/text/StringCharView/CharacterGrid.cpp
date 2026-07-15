// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// `StringCharView` is a specialized interface for code-point indexed access.
/// It is useful when text is organized by character positions, for example in
/// grids, terminal layouts, diagnostics, or editor columns.
/// Prefer byte indexes for general parsing, searching, and slicing UTF-8 text.
void characterGrid() {

    const auto grid = el::StringView{"AΩBçDÉFGH×\n"
                                     "IJKLMNÖPQR\n"
                                     "STÜVWXYZ01\n"
                                     "23456789ab\n"
                                     "cdefghijkl\n"
                                     "mnopqrstuv\n"
                                     "wxyzäöüß+-\n"
                                     "∑∏√∞≈≠≤≥÷·\n"
                                     "ABCDEFGHIJ\n"
                                     "UVWXYZ!?._\n"_el};

    auto gridCV = grid.toCharView();

    constexpr auto sourceWidth = el::CpLength{11};
    constexpr auto targetWidth = el::CpLength{10};
    constexpr auto height = el::CpLength{10};

    // Rotate the 10x10 character grid clockwise.
    auto rotatedGrid = el::String{};
    for (auto y = el::CpIndex{0}; y.isWithin(height); ++y) {
        for (auto x = el::CpIndex{0}; x.isWithin(targetWidth); ++x) {
            const auto sourceIndex = el::CpIndex::fromGrid(y, x.flipped(targetWidth), sourceWidth, height);
            rotatedGrid.append(gridCV.charAt(sourceIndex));
        }
        rotatedGrid.append(U'\n');
    }

    el::io::printLine("Original 10x10 character grid:"_el);
    el::io::printLine(grid);

    el::io::printLine("Rotated clockwise:"_el);
    el::io::printLine(rotatedGrid);
}

}
