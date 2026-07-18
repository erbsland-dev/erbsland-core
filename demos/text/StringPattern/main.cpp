// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "StringPatternDemos.hpp"

#include <DemoCommon.hpp>

using namespace demo;

auto main(const int argc, char *argv[]) -> int {
    auto app = DemoApplication{argc, argv};
    app.registerDemo("PatternConstruction"_el, patternConstruction);
    app.registerDemo("StaticConstruction"_el, staticConstruction);
    app.registerDemo("Matches"_el, matches);
    app.registerDemo("TrimAndTrimmed"_el, trimAndTrimmed);
    app.registerDemo("Split"_el, split);
    app.registerDemo("LengthAndIndex"_el, lengthAndIndex);
    return app.run();
}
