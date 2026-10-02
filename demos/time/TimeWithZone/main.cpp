// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "TimeWithZoneDemos.hpp"

#include <DemoCommon.hpp>

using namespace demo;

auto main(const int argc, char *argv[]) -> int {
    auto app = DemoApplication{argc, argv};
    app.registerDemo("Create"_el, create);
    app.registerDemo("Display"_el, display);
    app.registerDemo("Recurrence"_el, recurrence);
    app.registerDemo("Resolve"_el, resolve);
    app.registerDemo("Transitions"_el, transitions);
    return app.run();
}
