// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "DiagnosticDemos.hpp"

#include <DemoCommon.hpp>

namespace demo {

auto main(const int argc, char *argv[]) -> int {
    auto app = DemoApplication{argc, argv};
    app.enableTerminal();
    app.registerDemo("BuildDiagnosticDocument"_el, buildDiagnosticDocument);
    app.registerDemo("InspectDiagnostic"_el, inspectDiagnostic);
    app.registerDemo("RenderDiagnosticDocument"_el, renderDiagnosticDocument);
    return app.run();
}

}

auto main(const int argc, char *argv[]) -> int {
    return demo::main(argc, argv);
}
