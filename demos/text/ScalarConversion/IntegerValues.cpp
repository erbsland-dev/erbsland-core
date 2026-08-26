// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ScalarConversionDemos.hpp"

#include <cstdint>

namespace demo {

/// Parse complete integer values and format them for protocol and display use.
void integerValues() {
    const auto nodeId = el::String{"0x2A"_el}.toIntegerOrThrow<unsigned int>();

    auto groupedOptions = el::IntegerParseOptions::stringDefault();
    groupedOptions.addFlags(el::IntegerParseFlag::AllowSeparator);
    const auto sampleCount = el::String{"12'500"_el}.toIntegerOrThrow<int>(groupedOptions);
    const auto invalidPort = el::String{"70000"_el}.toInteger<uint16_t>(uint16_t{9000U});

    el::io::printLine("Node id ...........: "_el, nodeId);
    el::io::printLine("Measurements ......: "_el, sampleCount);
    el::io::printLine("Invalid port ......: "_el, invalidPort, " (fallback)"_el);

    try {
        const auto unexpectedValue = el::String{"12x"_el}.toIntegerOrThrow<int>();
        el::io::printLine("12x ...............: "_el, unexpectedValue);
    } catch (const el::Exception &) {
        el::io::printLine("12x ...............: parse error"_el);
    }

    auto hexadecimal = el::IntegerFormat::hexadecimal();
    hexadecimal.addFlags(el::IntegerFormatFlag::BasePrefix | el::IntegerFormatFlag::ZeroFill)
        .setLetterCase(el::LetterCase::Uppercase)
        .setFieldWidth(el::CpLength{6U});

    auto groupedDecimal = el::IntegerFormat::decimal();
    groupedDecimal.addFlags(el::IntegerFormatFlag::Separator).setSignMode(el::IntegerSignMode::Always);

    el::io::printLine("Hexadecimal .......: "_el, el::String::fromInteger(nodeId, hexadecimal));
    el::io::printLine("Grouped ...........: "_el, el::String::fromInteger(sampleCount, groupedDecimal));
}

}
