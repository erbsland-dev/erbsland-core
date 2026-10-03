// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/Application.hpp>
#include <erbsland/ExitCode.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/Options.hpp>
#include <erbsland/OptionType.hpp>
#include <erbsland/OptionValues.hpp>
#include <erbsland/pattern/Text.hpp>
#include <erbsland/render/Environment.hpp>
#include <erbsland/StandardStreams.hpp>
#include <erbsland/Version.hpp>

namespace demo {

using namespace el::text::literals;

/// Configure a small application with functions when a dedicated class would add no useful structure.
///
/// The initialization function runs at the beginning of `Application::run()`.
/// The main function runs after command-line parsing succeeds and can use the parsed option values.
auto main(const int argc, char *argv[]) -> int {
    auto app = el::Application{argc, argv};
    app.setInitializeFn([&app]() -> void {
        app.info().setApplicationName("Deployment Label"_el);
        app.info().setApplicationVersion(el::Version{1, 0, 0});
        app.options()
            ->addOption({"-e"_el, "--environment"_el, "environment"_el})
            .setType(el::OptionType::Text)
            .setDefaultValue("staging"_el)
            .setHelpTitle("Environment written into the deployment label."_el);
    });
    app.setMainFn([&app]() -> el::ExitCode {
        el::io::printLine("deployment environment: "_el, app.optionValues()->getText("environment"_el));
        return el::ExitCode::success();
    });
    return app.run();
}

}

auto main(const int argc, char *argv[]) -> int {
    return demo::main(argc, argv);
}
