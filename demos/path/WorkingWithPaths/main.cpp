// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "WorkingWithPathsDemos.hpp"

#include <DemoCommon.hpp>

using namespace demo;

auto main(const int argc, char *argv[]) -> int {
    auto app = DemoApplication{argc, argv};
    app.registerDemo("ConstructionAndFormats"_el, constructionAndFormats);
    app.registerDemo("InspectingPaths"_el, inspectingPaths);
    app.registerDemo("EditingPathNames"_el, editingPathNames);
    app.registerDemo("JoiningAndSlicing"_el, joiningAndSlicing);
    app.registerDemo("ConvertingPaths"_el, convertingPaths);
    return app.run();
}
