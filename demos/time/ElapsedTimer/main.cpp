// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ElapsedTimerDemos.hpp"

#include <DemoCommon.hpp>

using namespace demo;

auto main(const int argc, char *argv[]) -> int {
    auto app = DemoApplication{argc, argv};
    app.registerDemo("Budget"_el, budget);
    app.registerDemo("Measure"_el, measure);
    app.registerDemo("Phases"_el, phases);
    return app.run();
}
