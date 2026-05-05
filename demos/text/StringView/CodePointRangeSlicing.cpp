// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

/// Code-point range slicing is useful for short, fixed-shape text where the
/// positions are naturally counted in decoded characters.
///
/// For UTF-8 and UTF-16 strings, a code-point range requires scanning from the
/// beginning of the text to find the matching storage positions. Use it for
/// small identifiers and labels, not as an inner-loop strategy for large
/// documents.
void codePointRangeSlicing() {
    const auto label = el::StringView{"Färd-Karta-07"_el};

    // The label is short and fixed-shape, so code-point positions are readable.
    const auto mapName = label.slice(el::CpRange{el::CpIndex{5U}, el::CpLength{5U}});
    const auto number = label.slice(el::CpRange{el::CpIndex{11U}, el::CpLength{2U}});

    el::io::printLine("Label: "_el, label);
    el::io::printLine("Map name: "_el, mapName);
    el::io::printLine("Map number: "_el, number);
    el::io::printLine("Map name byte length: "_el, mapName.length());
}
