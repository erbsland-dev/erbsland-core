// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ConfigurationDocumentsDemos.hpp"

#include <DemoCommon.hpp>

using namespace demo;

auto main(const int argc, char *argv[]) -> int {
    auto app = DemoApplication{argc, argv};
    app.enableTerminal();
    app.options()
        ->addOption("configuration-file"_el)
        .setHelpDescription("Configuration document to parse and validate."_el);
    app.registerDemo("CompiledRules"_el, [&app]() -> void {
        compiledRules(el::Path{app.optionValues()->getText("configuration-file"_el)});
    });
    app.registerDemo("DocumentRules"_el, [&app]() -> void {
        documentRules(el::Path{app.optionValues()->getText("configuration-file"_el)});
    });
    app.registerDemo("LoadingStrategies"_el, loadingStrategies);
    app.registerDemo("ManualValidation"_el, [&app]() -> void {
        manualValidation(el::Path{app.optionValues()->getText("configuration-file"_el)});
    });
    app.registerDemo("ParseAndAccess"_el, parseAndAccess);
    return app.run();
}
