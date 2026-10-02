// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "TimestampDemos.hpp"

#include <DemoCommon.hpp>

using namespace demo;

auto main(const int argc, char *argv[]) -> int {
    auto app = DemoApplication{argc, argv};
    app.registerDemo("Calculate"_el, calculate);
    app.registerDemo("Create"_el, create);
    app.registerDemo("DateTime"_el, dateTime);
    app.registerDemo("Distances"_el, distances);
    app.registerDemo("Storage"_el, storage);
    app.registerDemo("Text"_el, text);
    app.registerDemo("Ticks"_el, ticks);
    return app.run();
}
