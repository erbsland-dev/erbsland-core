// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "PlaceholdersDemos.hpp"

#include <DemoCommon.hpp>

using namespace demo;

auto main(const int argc, char *argv[]) -> int {
    auto app = DemoApplication{argc, argv};
    app.enableTerminal();
    app.registerDemo("BuiltInProviders"_el, builtInProviders);
    app.registerDemo("BuiltInEnvironmentSource"_el, builtInEnvironmentSource);
    app.registerDemo("BuiltInVariableSource"_el, builtInVariableSource);
    app.registerDemo("BuiltInFilterValidation"_el, builtInFilterValidation);
    app.registerDemo("BuiltInTextConditions"_el, builtInTextConditions);
    app.registerDemo("BuiltInTextTransforms"_el, builtInTextTransforms);
    app.registerDemo("CustomFilter"_el, customFilter);
    app.registerDemo("CustomSource"_el, customSource);
    app.registerDemo("FilterError"_el, filterError);
    app.registerDemo("SourceError"_el, sourceError);
    return app.run();
}
