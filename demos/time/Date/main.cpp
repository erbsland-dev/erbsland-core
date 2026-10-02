// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "DateDemos.hpp"

#include <DemoCommon.hpp>

using namespace demo;

auto main(const int argc, char *argv[]) -> int {
    auto app = DemoApplication{argc, argv};
    app.registerDemo("Boundaries"_el, boundaries);
    app.registerDemo("Calculate"_el, calculate);
    app.registerDemo("Convert"_el, convert);
    app.registerDemo("CreateChecked"_el, createChecked);
    app.registerDemo("CreateTyped"_el, createTyped);
    app.registerDemo("Inspect"_el, inspect);
    return app.run();
}
