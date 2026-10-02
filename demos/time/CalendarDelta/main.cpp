// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "CalendarDeltaDemos.hpp"

#include <DemoCommon.hpp>

using namespace demo;

auto main(const int argc, char *argv[]) -> int {
    auto app = DemoApplication{argc, argv};
    app.registerDemo("Apply"_el, apply);
    app.registerDemo("Boundaries"_el, boundaries);
    app.registerDemo("Combine"_el, combine);
    app.registerDemo("Compose"_el, compose);
    app.registerDemo("Convert"_el, convert);
    app.registerDemo("Zones"_el, zones);
    return app.run();
}
