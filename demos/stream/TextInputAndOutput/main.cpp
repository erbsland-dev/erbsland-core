// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "TextInputAndOutputDemos.hpp"

#include <DemoCommon.hpp>

using namespace demo;

auto main(const int argc, char *argv[]) -> int {
    auto app = DemoApplication{argc, argv};
    app.registerDemo("CaptureReport"_el, captureReport);
    app.registerDemo("ConfigureEncoding"_el, configureEncoding);
    app.registerDemo("HandleInvalidEncoding"_el, handleInvalidEncoding);
    app.registerDemo("ReadObservationLines"_el, readObservationLines);
    app.registerDemo("ReadUnicodeBlocks"_el, readUnicodeBlocks);
    app.registerDemo("WriteGrowthReport"_el, writeGrowthReport);
    return app.run();
}
