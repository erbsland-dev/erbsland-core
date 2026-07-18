// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "StandardStreamsDemos.hpp"

using namespace demo;

auto main(const int argc, char *argv[]) -> int {
    auto app = DemoApplication{argc, argv};
    app.registerDemo("WriteStandardOutput"_el, writeStandardOutput);
    app.registerDemo("WriteStandardError"_el, writeStandardError);
    app.registerDemo("ReadRedirectedInput"_el, readRedirectedInput);
    app.registerDemo("CaptureOutput"_el, captureOutput);
    app.registerDemo("UseNestedRedirects"_el, useNestedRedirects);
    return app.run();
}
