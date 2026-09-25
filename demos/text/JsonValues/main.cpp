// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "JsonValuesDemos.hpp"

#include <DemoCommon.hpp>

using namespace demo;

auto main(const int argc, char *argv[]) -> int {
    auto app = DemoApplication{argc, argv};
    app.registerDemo("ParseDocument"_el, parseDocument);
    app.registerDemo("InspectDocument"_el, inspectDocument);
    app.registerDemo("BuildDocument"_el, buildDocument);
    app.registerDemo("FormatDocument"_el, formatDocument);
    app.registerDemo("LimitDocument"_el, limitDocument);
    return app.run();
}
