// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "IndividualValuesDemos.hpp"

#include <DemoCommon.hpp>

using namespace demo;

auto main(const int argc, char *argv[]) -> int {
    auto app = DemoApplication{argc, argv};
    app.registerDemo("ScalarConversions"_el, scalarConversions);
    app.registerDemo("TypeTests"_el, typeTests);
    app.registerDemo("TypedCollections"_el, typedCollections);
    app.registerDemo("TypedGetters"_el, typedGetters);
    app.registerDemo("ValueLists"_el, valueLists);
    return app.run();
}
