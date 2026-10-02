// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "IsoTimeOutputDemos.hpp"

#include <DemoCommon.hpp>

using namespace demo;

auto main(const int argc, char *argv[]) -> int {
    auto app = DemoApplication{argc, argv};
    app.registerDemo("CompleteOffset"_el, completeOffset);
    app.registerDemo("Defaults"_el, defaults);
    app.registerDemo("Extended"_el, extended);
    app.registerDemo("Fractions"_el, fractions);
    app.registerDemo("OffsetSeconds"_el, offsetSeconds);
    app.registerDemo("Precision"_el, precision);
    app.registerDemo("TimePrefix"_el, timePrefix);
    app.registerDemo("TimeShift"_el, timeShift);
    return app.run();
}
