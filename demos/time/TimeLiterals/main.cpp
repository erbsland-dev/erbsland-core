// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "TimeLiteralsDemos.hpp"

#include <DemoCommon.hpp>

using namespace demo;

auto main(const int argc, char *argv[]) -> int {
    auto app = DemoApplication{argc, argv};
    app.registerDemo("Expressions"_el, expressions);
    app.registerDemo("Limits"_el, limits);
    app.registerDemo("Scope"_el, scope);
    app.registerDemo("Units"_el, units);
    return app.run();
}
