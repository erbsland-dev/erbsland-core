// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "TimeDeltaDemos.hpp"

#include <DemoCommon.hpp>

using namespace demo;

auto main(const int argc, char *argv[]) -> int {
    auto app = DemoApplication{argc, argv};
    app.registerDemo("Boundaries"_el, boundaries);
    app.registerDemo("Calculate"_el, calculate);
    app.registerDemo("Convert"_el, convert);
    app.registerDemo("Create"_el, create);
    app.registerDemo("Display"_el, display);
    app.registerDemo("Factories"_el, factories);
    return app.run();
}
