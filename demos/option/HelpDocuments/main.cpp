// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/all.hpp>
#include <erbsland/i18n/DisplayTextMap.hpp>

namespace demo {

using namespace el::text::literals;

/// `OptionManager` can build neutral help, version, and error documents without using `Application`.
///
/// The documents are `TextDocument` trees.
/// A command line application can display them with the default plain renderer, or with terminal styling after enabling
/// terminal support.
auto helpDocuments() -> el::ExitCode {
    auto options = el::Options::create();
    auto info = el::ApplicationInfo{};
    info.setApplicationName("Instrument Help"_el);
    info.setApplicationVersion(el::Version{0, 8, 0});
    options->setApplicationInfo(info);
    options->setExecutablePath("/usr/local/bin/instrument-help"_el);
    options->setHelpTitle("Instrument Help"_el);
    options->setHelpDescription("Renders option help as a text document."_el);
    options->addOption({"-m"_el, "--mode"_el, "mode"_el})
        .addChoice("plain"_el)
        .addChoice("terminal"_el)
        .setHelpDescription("Rendering style used by the enclosing application."_el);
    options->addOption("topic"_el).setHelpDescription("Help topic to inspect."_el);

    auto displayText = el::DisplayTextMap::defaultMap()->clone();
    displayText->set("options.OptionsHeading"_el, "Registered Options"_el);
    auto manager = el::OptionManager{options};
    manager.setDisplayTextMap(displayText);

    el::io::printLine(manager.helpDocument({}).toString());
    return el::ExitCode::success();
}

}

auto main(const int argc, char *argv[]) -> int {
    auto app = el::Application{argc, argv};
    app.enableTerminal();
    app.setMainFn(demo::helpDocuments);
    return app.run();
}
