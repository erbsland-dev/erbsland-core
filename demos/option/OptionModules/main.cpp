// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/all.hpp>

namespace demo {

using namespace el::text::literals;

/// Use `OptionModule` for command-style tools where the first argument selects an action.
///
/// A module owns its own option sets and can provide a main function.
/// `Application` calls the selected module main function automatically after parsing succeeds.
class InstrumentArchiveApp final : public el::Application {
public:
    using Application::Application;

protected:
    void initialize() override {
        enableTerminal();
        info().setApplicationName("Instrument Archive"_el);
        info().setApplicationVersion(el::Version{0, 8, 0});
        info().setLicenseText("Apache-2.0"_el);
    }
    void registerCommandLineOptions(const el::OptionsPtr &options) override {
        options->setHelpTitle("Instrument Archive"_el);
        options->setHelpDescription("Manages small records for scientific instruments."_el);
        options->addOption({"-v"_el, "--verbose"_el, "verbose"_el})
            .setHelpDescription("Prints additional progress details."_el)
            .setHelpVisibility(el::OptionHelpVisibility::Overview);

        auto scan = el::OptionModule::create("scan"_el);
        scan->setHelpTitle("Scan Instrument"_el);
        scan->setHelpDescription("Scans an instrument and stores its readings."_el);
        scan->addOption({"-r"_el, "--range"_el, "range"_el})
            .setType(el::OptionType::Integer)
            .setDefaultValue(el::OptionInteger{5})
            .setHelpDescription("Reading range around the starting point."_el);
        scan->addOption("instrument"_el).setRequired().setHelpDescription("Instrument to scan."_el);
        scan->setMainFn([](const el::OptionValuesPtr &values) -> el::ExitCode {
            el::io::printLine("module: "_el, values->moduleName());
            el::io::printLine("instrument: "_el, values->getText("instrument"_el));
            el::io::printLine("range: "_el, values->getInteger("range"_el));
            el::io::printLine("verbose: "_el, el::BooleanFormat::yesNo(), values->getFlag("verbose"_el));
            return el::ExitCode::success();
        });
        options->addModule(scan);

        auto render = el::OptionModule::create("render"_el);
        render->setHelpTitle("Render Record"_el);
        render->setHelpDescription("Renders a record as text, a chart, or data."_el);
        render->addOption({"-f"_el, "--format"_el, "format"_el})
            .addChoice("text"_el)
            .addChoice("chart"_el)
            .addChoice("data"_el)
            .setDefaultValue("text"_el)
            .setHelpDescription("Output format for the record."_el);
        render->addOption("instrument"_el).setRequired().setHelpDescription("Instrument to render."_el);
        render->setMainFn([](const el::OptionValuesPtr &values) -> el::ExitCode {
            el::io::printLine("module: "_el, values->moduleName());
            el::io::printLine("instrument: "_el, values->getText("instrument"_el));
            el::io::printLine("format: "_el, values->getText("format"_el));
            return el::ExitCode::success();
        });
        options->addModule(render);
    }
};

/// This main method leaves module dispatch to `Application`.
auto main(const int argc, char *argv[]) -> int {
    auto app = InstrumentArchiveApp{argc, argv};
    return app.run();
}

}

auto main(const int argc, char *argv[]) -> int {
    return demo::main(argc, argv);
}
