// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "TimeDemos.hpp"

#include <DemoCommon.hpp>

using namespace demo;

auto main(const int argc, char *argv[]) -> int {
    auto app = DemoApplication{argc, argv};
    app.registerDemo("Convert"_el, convert);
    app.registerDemo("Create"_el, create);
    app.registerDemo("Inspect"_el, inspect);
    app.registerDemo("Wrap"_el, wrap);
    return app.run();
}
