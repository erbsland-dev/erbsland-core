// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "TextDocumentDemos.hpp"

#include <DemoCommon.hpp>
#include <DemoTerminal.hpp>

using namespace demo;

auto main(const int argc, char *argv[]) -> int {
    auto app = DemoApplication{argc, argv};
    DemoTerminal::enable(app);
    app.registerDemo("RenderCodeSnippet"_el, renderCodeSnippet);
    return app.run();
}
