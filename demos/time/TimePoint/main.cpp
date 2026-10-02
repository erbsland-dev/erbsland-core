// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "TimePointDemos.hpp"

#include <DemoCommon.hpp>

using namespace demo;

auto main(const int argc, char *argv[]) -> int {
    auto app = DemoApplication{argc, argv};
    app.registerDemo("Checkpoints"_el, checkpoints);
    app.registerDemo("Deadline"_el, deadline);
    app.registerDemo("Interop"_el, interop);
    app.registerDemo("Intervals"_el, intervals);
    return app.run();
}
