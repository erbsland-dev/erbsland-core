// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StandardStreamsDemos.hpp"


void integerFormatting() {
    const auto plotId = 0x2afU;
    auto hexadecimal = el::IntegerFormat::hexadecimal();
    hexadecimal.setFlags(el::IntegerFormatFlag::BasePrefix).setLetterCase(el::LetterCase::Uppercase);

    auto groupedDecimal = el::IntegerFormat::decimal();
    groupedDecimal.setFlags(el::IntegerFormatFlag::Separator);

    el::io::printLine("Plot id decimal: "_el, plotId);
    el::io::printLine("Plot id hexadecimal: "_el, hexadecimal, plotId);
    el::io::printLine("Estimated seedlings: "_el, groupedDecimal, 125000U);
}
