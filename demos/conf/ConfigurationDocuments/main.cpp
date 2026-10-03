// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ConfigurationDocumentsDemos.hpp"

#include <DemoCommon.hpp>
#include <DemoTerminal.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/OptionValues.hpp>
#include <erbsland/Path.hpp>

using namespace demo;

auto main(const int argc, char *argv[]) -> int {
    constexpr static auto cConfigurationFileOption = "configuration-file"_el;

    auto app = DemoApplication{argc, argv};
    DemoTerminal::enable(app);
    app.options()->addOption(cConfigurationFileOption).setHelpTitle("Configuration document to parse and validate."_el);
    app.registerDemo("CompiledRules"_el, [&app]() -> void {
        compiledRules(el::Path{app.optionValues()->getText(cConfigurationFileOption)});
    });
    app.registerDemo("DocumentRules"_el, [&app]() -> void {
        documentRules(el::Path{app.optionValues()->getText(cConfigurationFileOption)});
    });
    app.registerDemo("LoadingStrategies"_el, loadingStrategies);
    app.registerDemo("ManualValidation"_el, [&app]() -> void {
        manualValidation(el::Path{app.optionValues()->getText(cConfigurationFileOption)});
    });
    app.registerDemo("ParseAndAccess"_el, parseAndAccess);
    return app.run();
}
