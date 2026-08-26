// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ScalarConversionDemos.hpp"

namespace demo {

/// Parse floating-point measurements and select their output notation and precision.
void floatingPointValues() {
    const auto humidity = el::String{"48.75"_el}.toFloatOrThrow<double>();

    auto scientificInput = el::FloatParseOptions{};
    scientificInput.setStyle(el::FloatParseOptions::Style::Scientific);
    const auto pressure = el::String{"1.013e3"_el}.toFloatOrThrow<double>(scientificInput);

    auto valueWithUnit = el::FloatParseOptions{};
    valueWithUnit.addFlags(el::FloatParseFlag::IgnoreTrailingChars);
    const auto temperature = el::String{"21.5 °C"_el}.toFloatOrThrow<double>(valueWithUnit);
    const auto missing = el::String{"geen meting"_el}.toFloat<double>(-1.0);

    el::io::printLine("Humidity ..........: "_el, humidity);
    el::io::printLine("Pressure ..........: "_el, pressure);
    el::io::printLine("Temperature .......: "_el, temperature);
    el::io::printLine("Missing ...........: "_el, missing, " (fallback)"_el);

    try {
        const auto unexpectedValue = el::String{"21,5"_el}.toFloatOrThrow<double>();
        el::io::printLine("21,5 ..............: "_el, unexpectedValue);
    } catch (const el::Exception &) {
        el::io::printLine("21,5 ..............: parse error"_el);
    }

    auto fixed = el::FloatFormat::fixed();
    fixed.setPrecision(el::ItemCount{2U});
    auto scientific = el::FloatFormat::scientific();
    scientific.setPrecision(el::ItemCount{3U}).setLetterCase(el::LetterCase::Uppercase);

    el::io::printLine("Fixed format ......: "_el, el::String::fromFloat(humidity, fixed));
    el::io::printLine("Scientific ........: "_el, el::String::fromFloat(pressure, scientific));
}

}
