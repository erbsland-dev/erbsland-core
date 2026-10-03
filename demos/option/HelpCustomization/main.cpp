// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoTerminal.hpp>
#include <DisplayTextDemo.hpp>
#include <erbsland/Application.hpp>
#include <erbsland/ApplicationInfo.hpp>
#include <erbsland/cterm/TerminalDocumentStyle.hpp>
#include <erbsland/DisplayText.hpp>
#include <erbsland/ExitCode.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/OptionHelpVisibility.hpp>
#include <erbsland/OptionManager.hpp>
#include <erbsland/Options.hpp>
#include <erbsland/OptionType.hpp>
#include <erbsland/pattern/Text.hpp>
#include <erbsland/StandardStreams.hpp>
#include <erbsland/TextDocument.hpp>
#include <erbsland/Version.hpp>

#include <string_view>

namespace demo {

using namespace el::text::literals;

void addSharedOptions(const el::OptionsPtr &options) {
    options->setHelpTitle("Night Watch Help"_el);
    options->setHelpDescription("Shows how option help can be rendered as a document."_el);
    options->addOption({"-g"_el, "--area"_el, "area"_el})
        .setType(el::OptionType::Text)
        .setValueName("area"_el)
        .setHelpTitle("Area for which to view help."_el);
    options->addOption({"-f"_el, "--format"_el, "format"_el})
        .addChoice("short"_el)
        .addChoice("full"_el)
        .setDefaultValue("short"_el)
        .setHelpTitle("Output detail level."_el);
}

/// `Application::enableTerminal()` switches generated help from plain text to styled terminal output.
///
/// This application checks for `--terminal` before parsing and enables terminal rendering early enough for `--help`.
/// It also sets a predefined terminal document style for system output.
class HelpCustomizationApp final : public el::Application {
public:
    using Application::Application;

protected:
    void initialize() override {
        info().setApplicationName("Night Watch Help"_el);
        info().setApplicationVersion(el::Version{0, 8, 0});
        for (const auto &arg : commandLineArguments()) {
            if (arg == "--terminal"_el) {
                DemoTerminal::enable(*this);
                setSystemOutputStyle(el::cterm::TerminalDocumentStyle::defaultStyled());
                break;
            }
        }
    }
    void registerCommandLineOptions(const el::OptionsPtr &options) override {
        addSharedOptions(options);
        options->addOption({"--terminal"_el, "terminal"_el})
            .setHelpTitle("Renders help with terminal colors."_el)
            .setHelpVisibility(el::OptionHelpVisibility::Hidden);
    }
};

/// `OptionManager` exposes neutral help documents and compiled display-text layers for manual rendering and wording
/// changes.
///
/// The manager can build a `TextDocument` without owning the application lifecycle.
/// Change compiled display-text layers when generated labels such as `Usage`, `Options`, placeholders, or built-in
/// option descriptions need application-specific wording.
auto manualHelpRendering() -> el::ExitCode {
    auto options = el::Options::create();
    auto info = el::ApplicationInfo{};
    info.setApplicationName("Manual Help"_el);
    info.setApplicationVersion(el::Version{0, 8, 0});
    options->setApplicationInfo(info);
    options->setExecutablePath("manual-help"_el);
    addSharedOptions(options);

    el::application().displayText()->addData(DisplayTextDemo::create());

    auto manager = el::OptionManager{options};
    el::io::printLine(manager.helpDocument({}).toString());
    return el::ExitCode::success();
}

}

auto main(const int argc, char *argv[]) -> int {
    if (argc > 1 && std::string_view{argv[1]} == std::string_view{"manual"}) {
        auto app = el::Application{};
        app.setMainFn(demo::manualHelpRendering);
        return app.run();
    }
    auto app = demo::HelpCustomizationApp{argc, argv};
    return app.run();
}
