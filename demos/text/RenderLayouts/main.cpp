// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "RenderLayoutsDemos.hpp"

#include <DemoCommon.hpp>

using namespace demo;

auto main(const int argc, char *argv[]) -> int {
    auto app = DemoApplication{argc, argv};
    app.registerDemo("BlockDefaults"_el, blockDefaults);
    app.registerDemo("ChainedInheritance"_el, chainedInheritance);
    app.registerDemo("ContextValues"_el, contextValues);
    app.registerDemo("CustomFilter"_el, customFilter);
    app.registerDemo("CustomLoader"_el, customLoader);
    app.registerDemo("CustomSyntax"_el, customSyntax);
    app.registerDemo("EscapingOptions"_el, escapingOptions);
    app.registerDemo("FeatureCheatSheet"_el, featureCheatSheet);
    app.registerDemo("FileLoader"_el, fileLoader);
    app.registerDemo("Introduction"_el, introduction);
    app.registerDemo("LanguageConditions"_el, languageConditions);
    app.registerDemo("LanguageExpressions"_el, languageExpressions);
    app.registerDemo("LanguageIterations"_el, languageIterations);
    app.registerDemo("LayeredLoaders"_el, layeredLoaders);
    app.registerDemo("LayoutInheritance"_el, layoutInheritance);
    app.registerDemo("LazyValue"_el, lazyValue);
    app.registerDemo("LocalValues"_el, localValues);
    app.registerDemo("RenderingLimit"_el, renderingLimit);
    app.registerDemo("RenderWorkflow"_el, renderWorkflow);
    app.registerDemo("ResourceLoader"_el, resourceLoader);
    app.registerDemo("SharedValues"_el, sharedValues);
    app.registerDemo("ValueShapes"_el, valueShapes);
    return app.run();
}
