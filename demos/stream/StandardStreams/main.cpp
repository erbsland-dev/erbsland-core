// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "StandardStreamsDemos.hpp"

auto main(const int argc, char *argv[]) -> int {
    auto app = DemoApplication{argc, argv};
    app.registerDemo("EasyOutput"_el, easyOutput);
    app.registerDemo("StandardOutputAndError"_el, standardOutputAndError);
    app.registerDemo("IntegerFormatting"_el, integerFormatting);
    app.registerDemo("FloatFormatting"_el, floatFormatting);
    app.registerDemo("LowLevelWrite"_el, lowLevelWrite);
    return app.run();
}
