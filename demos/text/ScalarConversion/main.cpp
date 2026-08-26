// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ScalarConversionDemos.hpp"

#include <DemoCommon.hpp>

using namespace demo;

auto main(const int argc, char *argv[]) -> int {
    auto app = DemoApplication{argc, argv};
    app.registerDemo("BooleanValues"_el, booleanValues);
    app.registerDemo("FloatingPointValues"_el, floatingPointValues);
    app.registerDemo("IntegerValues"_el, integerValues);
    return app.run();
}
