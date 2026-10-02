// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "TimeZoneDemos.hpp"

#include <DemoCommon.hpp>

using namespace demo;

auto main(const int argc, char *argv[]) -> int {
    auto app = DemoApplication{argc, argv};
    app.registerDemo("Convert"_el, convert);
    app.registerDemo("Create"_el, create);
    app.registerDemo("Database"_el, database);
    app.registerDemo("Details"_el, details);
    app.registerDemo("Inspect"_el, inspect);
    app.registerDemo("Local"_el, local);
    app.registerDemo("Resolve"_el, resolve);
    app.registerDemo("Transitions"_el, transitions);
    return app.run();
}
