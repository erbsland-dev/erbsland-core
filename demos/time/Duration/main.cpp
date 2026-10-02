// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "DurationDemos.hpp"

#include <DemoCommon.hpp>

using namespace demo;

auto main(const int argc, char *argv[]) -> int {
    auto app = DemoApplication{argc, argv};
    app.registerDemo("Calculate"_el, calculate);
    app.registerDemo("Convert"_el, convert);
    app.registerDemo("Create"_el, create);
    app.registerDemo("Parts"_el, parts);
    app.registerDemo("Precise"_el, precise);
    return app.run();
}
