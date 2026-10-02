// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "TimeDeltaOutputDemos.hpp"

#include <DemoCommon.hpp>

using namespace demo;

auto main(const int argc, char *argv[]) -> int {
    auto app = DemoApplication{argc, argv};
    app.registerDemo("Calendar"_el, calendar);
    app.registerDemo("FractionDigits"_el, fractionDigits);
    app.registerDemo("Fractions"_el, fractions);
    app.registerDemo("Precision"_el, precision);
    app.registerDemo("Presets"_el, presets);
    app.registerDemo("Reuse"_el, reuse);
    app.registerDemo("SmallestUnit"_el, smallestUnit);
    app.registerDemo("UnitSeparator"_el, unitSeparator);
    app.registerDemo("UnitStyle"_el, unitStyle);
    app.registerDemo("ValueSeparator"_el, valueSeparator);
    return app.run();
}
