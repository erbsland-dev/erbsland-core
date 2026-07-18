// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "StreamErrorsDemos.hpp"

#include <DemoCommon.hpp>

using namespace demo;

auto main(const int argc, char *argv[]) -> int {
    auto app = DemoApplication{argc, argv};
    app.registerDemo("DistinguishResultStates"_el, distinguishResultStates);
    app.registerDemo("HandleEncodingError"_el, handleEncodingError);
    app.registerDemo("InspectStreamError"_el, inspectStreamError);
    app.registerDemo("PreserveWrappedContext"_el, preserveWrappedContext);
    app.registerDemo("ReportDiagnostic"_el, reportDiagnostic);
    return app.run();
}
