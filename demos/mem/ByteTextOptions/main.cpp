// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ByteTextOptionsDemos.hpp"

#include <DemoCommon.hpp>

using namespace demo;

auto main(const int argc, char *argv[]) -> int {
    auto app = DemoApplication{argc, argv};
    app.registerDemo("SelectCountFormat"_el, selectCountFormat);
    app.registerDemo("SelectEncoding"_el, selectEncoding);
    app.registerDemo("SelectFormat"_el, selectFormat);
    app.registerDemo("UseEndMark"_el, useEndMark);
    app.registerDemo("UsePadding"_el, usePadding);
    return app.run();
}
