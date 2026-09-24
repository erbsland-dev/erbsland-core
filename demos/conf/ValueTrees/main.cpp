// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ValueTreesDemos.hpp"

#include <DemoCommon.hpp>

using namespace demo;

auto main(const int argc, char *argv[]) -> int {
    auto app = DemoApplication{argc, argv};
    app.registerDemo("IteratingLists"_el, iteratingLists);
    app.registerDemo("NamePaths"_el, namePaths);
    app.registerDemo("NavigatingTree"_el, navigatingTree);
    app.registerDemo("ValueNames"_el, valueNames);
    return app.run();
}
