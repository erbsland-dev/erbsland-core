// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "StringFormatDemos.hpp"

#include <DemoCommon.hpp>

using namespace demo;

auto main(const int argc, char *argv[]) -> int {
    auto app = DemoApplication{argc, argv};
    app.registerDemo("EmptyPlaceholder"_el, emptyPlaceholder);
    app.registerDemo("EscapeAmount"_el, escapeAmount);
    app.registerDemo("EscapeFormat"_el, escapeFormat);
    app.registerDemo("FloatFormats"_el, floatFormats);
    app.registerDemo("FormattingPatterns"_el, formattingPatterns);
    app.registerDemo("IntegerFormats"_el, integerFormats);
    app.registerDemo("NamedBooleanFormat"_el, namedBooleanFormat);
    app.registerDemo("NamedByteFormat"_el, namedByteFormat);
    app.registerDemo("NamedFloatFormat"_el, namedFloatFormat);
    app.registerDemo("NamedIntegerFormat"_el, namedIntegerFormat);
    app.registerDemo("NamedTextFormat"_el, namedTextFormat);
    app.registerDemo("PositionalPlaceholders"_el, positionalPlaceholders);
    app.registerDemo("SupportedTypes"_el, supportedTypes);
    app.registerDemo("TextFormats"_el, textFormats);
    return app.run();
}
