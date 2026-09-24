// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "AccessChecksDemos.hpp"

#include <DemoCommon.hpp>

using namespace demo;

auto main(const int argc, char *argv[]) -> int {
    auto app = DemoApplication{argc, argv};
    app.registerDemo("ConfigureFileAccess"_el, configureFileAccess);
    app.registerDemo("CustomAccessCheck"_el, customAccessCheck);
    return app.run();
}
