// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "DateTimeDemos.hpp"

#include <DemoCommon.hpp>

using namespace demo;

auto main(const int argc, char *argv[]) -> int {
    auto app = DemoApplication{argc, argv};
    app.registerDemo("Calculate"_el, calculate);
    app.registerDemo("Compare"_el, compare);
    app.registerDemo("Create"_el, create);
    app.registerDemo("Inspect"_el, inspect);
    app.registerDemo("Recurrence"_el, recurrence);
    app.registerDemo("Text"_el, text);
    app.registerDemo("Ticks"_el, ticks);
    app.registerDemo("Zones"_el, zones);
    return app.run();
}
