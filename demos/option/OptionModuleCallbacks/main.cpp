// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/all.hpp>

namespace demo {

using namespace el::text::literals;

struct ModuleSettings {
    el::String laatsteModule;
    bool luid{false};
};

ModuleSettings settings;

[[nodiscard]] auto createObserveModule() -> el::OptionModulePtr {
    auto module = el::OptionModule::create("observe"_el);
    module->setHelpTitle("Observe Area"_el);
    module->setHelpDescription("Starts an observation round in a nighttime area."_el);
    module->setHelpEpilog("Example: night-watch observe --duration 45 dune-edge"_el);
    module->addOption({"-d"_el, "--duration"_el, "duration"_el})
        .setType(el::OptionType::Integer)
        .setDefaultValue(el::OptionInteger{30})
        .setHelpDescription("Duration of the round in minutes."_el);
    module->addOption("area"_el)
        .setRequired()
        .setHelpDescription("Area to observe."_el)
        .setHelpVisibility(el::OptionHelpVisibility::Usage);
    module->setPreParsingFn([](const el::OptionModulePtr &selectedModule) -> void {
        selectedModule->editOption("duration"_el)
            .setValidateFn([](const el::OptionValuePtr &value, el::OptionValuesPtr) {
                if (value->getInteger() < 10 || value->getInteger() > 120) {
                    auto context = el::OptionErrorContext{};
                    context.setDescription("The observation duration must be between 10 and 120 minutes."_el);
                    throw el::OptionError{context};
                }
            });
    });
    module->setPostParsingFn([](const el::OptionValuesPtr &values) -> void {
        settings.laatsteModule = values->moduleName();
        settings.luid = values->getFlag("loud"_el);
    });
    module->setMainFn([](const el::OptionValuesPtr &values) -> el::ExitCode {
        el::io::printLine("module: "_el, values->moduleName());
        el::io::printLine("area: "_el, values->getText("area"_el));
        el::io::printLine("duration: "_el, values->getInteger("duration"_el));
        el::io::printLine("loud: "_el, el::BooleanFormat::yesNo(), settings.luid);
        return el::ExitCode::success();
    });
    return module;
}

[[nodiscard]] auto createExportModule() -> el::OptionModulePtr {
    auto module = el::OptionModule::create("export"_el);
    module->setHelpTitle("Export Logbook"_el);
    module->setHelpDescription("Writes an existing observation as text or a table."_el);
    module->addOption({"-f"_el, "--format"_el, "format"_el})
        .addChoice("text"_el)
        .addChoice("table"_el)
        .setDefaultValue("text"_el)
        .setHelpDescription("File format for the export."_el);
    module->addOption("logbook"_el).setRequired().setHelpDescription("Name of the logbook."_el);
    module->setMainFn([](const el::OptionValuesPtr &values) -> el::ExitCode {
        el::io::printLine("module: "_el, values->moduleName());
        el::io::printLine("logbook: "_el, values->getText("logbook"_el));
        el::io::printLine("format: "_el, values->getText("format"_el));
        return el::ExitCode::success();
    });
    return module;
}

/// Option modules model command-style applications where the first ordinary argument selects an action.
///
/// Each module owns module-local options, callbacks, help metadata, and an optional main function.
/// Root options stay available after the module is selected.
/// `Application` automatically calls the selected module main function after parsing succeeds.
class NightWatchApp final : public el::Application {
public:
    using Application::Application;

protected:
    void initialize() override {
        enableTerminal();
        info().setApplicationName("Night Watch Modules"_el);
        info().setApplicationVersion(el::Version{0, 8, 0});
    }
    void registerCommandLineOptions(const el::OptionsPtr &options) override {
        options->setHelpTitle("Night Watch Modules"_el);
        options->setHelpDescription("Manages observation rounds through separate command modules."_el);
        options->addOption({"-l"_el, "--loud"_el, "loud"_el})
            .setHelpDescription("Prints extra progress information."_el)
            .setHelpVisibility(el::OptionHelpVisibility::Overview);
        options->addModule(createObserveModule());
        options->addModule(createExportModule());

        auto debug = el::OptionModule::create("debug"_el);
        debug->setHelpDescription("Hidden diagnostic module for developers."_el);
        debug->setHelpVisibility(el::OptionHelpVisibility::Hidden);
        debug->setMainFn([](el::OptionValuesPtr) -> el::ExitCode {
            el::io::printLine("debug module"_el);
            return el::ExitCode::success();
        });
        options->addModule(debug);
    }
};

}

auto main(const int argc, char *argv[]) -> int {
    auto app = demo::NightWatchApp{argc, argv};
    return app.run();
}
