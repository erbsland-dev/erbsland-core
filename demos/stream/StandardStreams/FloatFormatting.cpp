// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StandardStreamsDemos.hpp"


void floatFormatting() {
    auto fixed = el::FloatFormat::fixed();
    fixed.setPrecision(el::ElementCount{2U});

    auto scientific = el::FloatFormat::scientific();
    scientific.setPrecision(el::ElementCount{3U}).setLetterCase(el::LetterCase::Uppercase);

    el::io::printLine("Average canopy height: "_el, fixed, 18.756, " m"_el);
    el::io::printLine("Pollen sample density: "_el, scientific, 0.000421);
}
