// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "PlaceholderDemos.hpp"

using namespace demo;

auto main(const int argc, char *argv[]) -> int {
    auto app = DemoApplication{argc, argv};
    app.registerDemo("ReplaceGalleryLabel"_el, replaceGalleryLabel);
    app.registerDemo("UseBuiltInProviders"_el, useBuiltInProviders);
    app.registerDemo("UseCustomSource"_el, useCustomSource);
    app.registerDemo("UseCustomFilter"_el, useCustomFilter);
    app.registerDemo("ChooseSyntax"_el, chooseSyntax);
    app.registerDemo("ChooseEscapeMode"_el, chooseEscapeMode);
    app.registerDemo("HandleInvalidExpression"_el, handleInvalidExpression);
    app.registerDemo("ReadEnvironmentSource"_el, readEnvironmentSource);
    app.registerDemo("ReadVariableSource"_el, readVariableSource);
    app.registerDemo("TransformFieldNote"_el, transformFieldNote);
    app.registerDemo("ChooseFieldNoteText"_el, chooseFieldNoteText);
    app.registerDemo("RequireFieldNoteValue"_el, requireFieldNoteValue);
    return app.run();
}
