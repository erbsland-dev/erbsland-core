// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/all.hpp>

namespace demo {

using namespace el::text::literals;

[[nodiscard]] auto createExportChoices() -> el::OptionChoicesPtr {
    auto choices = el::OptionChoices::create();

    auto textHelp = el::OptionHelp{"Compact output for human review."_el};
    choices->addChoice(el::OptionChoice::create("text"_el, textHelp));

    auto tableHelp = el::OptionHelp{"Delimited table for spreadsheets."_el};
    choices->addChoice(el::OptionChoice::create("table"_el, tableHelp));

    auto dataHelp = el::OptionHelp{"Data structure for downstream tools."_el};
    choices->addChoice(el::OptionChoice::create("data"_el, dataHelp));

    auto internalHelp = el::OptionHelp{"Internal format used while debugging the renderer."_el};
    internalHelp.setVisibility(el::OptionHelpVisibility::Hidden);
    choices->addChoice(el::OptionChoice::create("internal"_el, internalHelp));
    return choices;
}

[[nodiscard]] auto createExecutionSet() -> el::OptionSetPtr {
    auto set = el::OptionSet::create();
    set->setHelpTitle("Execution"_el);
    set->setHelpDescription("Common controls for every selected module."_el);
    set->setHelpVisibility(el::OptionHelpVisibility::Overview);
    set->addOption({"-v"_el, "--verbose"_el, "verbose"_el})
        .setMaximum(el::ArgumentCount{4U})
        .setHelpDescription("Increases diagnostic detail. Can be repeated."_el)
        .setHelpVisibility(el::OptionHelpVisibility::Usage);
    set->addOption({"--config"_el, "config"_el})
        .setType(el::OptionType::Text)
        .setValueName("file"_el)
        .setHelpDescription("Main configuration file."_el)
        .setHelpVisibility(el::OptionHelpVisibility::Overview);
    set->addOption({"--workspace"_el, "workspace"_el})
        .setType(el::OptionType::Text)
        .setValueName("directory"_el)
        .setDefaultValue("laboratory"_el)
        .setHelpDescription("Working directory for instrument data."_el)
        .setHelpVisibility(el::OptionHelpVisibility::Overview);
    set->addOption({"--color"_el, "color"_el})
        .addChoice("auto"_el)
        .addChoice("always"_el)
        .addChoice("never"_el)
        .setValueName("mode"_el)
        .setDefaultValue("auto"_el)
        .setHelpDescription("Color control for compatible terminals."_el)
        .setHelpVisibility(el::OptionHelpVisibility::Overview);
    set->addOption({"--trace-options"_el, "trace-options"_el})
        .setHelpDescription("Traces option lookup and argument assignment."_el)
        .setHelpVisibility(el::OptionHelpVisibility::Hidden);
    return set;
}

[[nodiscard]] auto createReportSet() -> el::OptionSetPtr {
    auto set = el::OptionSet::create();
    set->setHelpTitle("Reports"_el);
    set->setHelpDescription("Global values used by modules that generate reports."_el);
    set->addOption({"--region"_el, "region"_el})
        .setType(el::OptionType::Text)
        .setValueName("area"_el)
        .setHelpDescription("Laboratory or measurement-campaign region."_el);
    set->addOption({"--label"_el, "label"_el})
        .setType(el::OptionType::Text)
        .setValueName("text"_el)
        .setMaximum(el::ArgumentCount{12U})
        .setHelpDescription("Additional label for generated records. Can be repeated."_el);
    set->addOption({"--note"_el, "note"_el})
        .setType(el::OptionType::Text)
        .setHelpDescription("Short note printed on the report cover."_el);
    set->addOption({"--width"_el, "width"_el})
        .setType(el::OptionType::Integer)
        .setDefaultValue(el::OptionInteger{120})
        .setHelpDescription("Preferred width of report pages."_el);
    set->addOption({"--height"_el, "height"_el})
        .setType(el::OptionType::Integer)
        .setDefaultValue(el::OptionInteger{80})
        .setHelpDescription("Preferred height of report pages."_el);
    set->addOption({"--scale"_el, "scale"_el})
        .addChoice("small"_el)
        .addChoice("medium"_el)
        .addChoice("large"_el)
        .setValueName("size"_el)
        .setDefaultValue("medium"_el)
        .setHelpDescription("Default visual scale for charts."_el);
    set->addOption({"--order"_el, "order"_el})
        .addChoice("name"_el)
        .addChoice("date"_el)
        .addChoice("instrument"_el)
        .setHelpDescription("Order of records in indexes."_el);
    set->addOption({"--filter"_el, "filter"_el})
        .setType(el::OptionType::Text)
        .setValueName("expression"_el)
        .setMaximum(el::ArgumentCount{6U})
        .setHelpDescription(
            "Filter expression for records. Can be repeated to combine conditions on instrument, date, operator, "
            "and validation status."_el);
    set->addOption({"--experimental-summary-template"_el, "experimental-summary-template"_el})
        .setType(el::OptionType::Text)
        .setValueName("template"_el)
        .setHelpDescription(
            "Long template for experimental-review reports; its deliberately wide name helps check how aligned "
            "descriptions wrap."_el);
    set->addOption({"--index"_el, "index"_el}).setHelpDescription("Builds an index page."_el);
    set->addOption({"--preview"_el, "preview"_el})
        .setHelpDescription("Generates a preview instead of the complete result."_el);
    return set;
}

[[nodiscard]] auto createDisabledLegacySet() -> el::OptionSetPtr {
    auto set = el::OptionSet::create();
    set->setHelpTitle("Legacy"_el);
    set->setHelpDescription("Old options that must no longer appear or be parsed."_el);
    set->setFlags(el::OptionFlag::Disabled);
    set->addOption({"--old-mode"_el, "old-mode"_el})
        .setHelpDescription("Retired option that must not exist for the parser."_el);
    return set;
}

void addCommonModuleOptions(const el::OptionModulePtr &module) {
    auto commonSet = el::OptionSet::create();
    commonSet->setHelpTitle("Module"_el);
    commonSet->setHelpDescription("Common controls for the selected module."_el);
    commonSet->addOption({"--profile"_el, "profile"_el})
        .addChoice("field"_el)
        .addChoice("file"_el)
        .addChoice("training"_el)
        .setValueName("profile"_el)
        .setDefaultValue("field"_el)
        .setHelpDescription("Execution profile for this module."_el);
    commonSet->addOption({"--jobs"_el, "jobs"_el})
        .setType(el::OptionType::Integer)
        .setValueName("processes"_el)
        .setDefaultValue(el::OptionInteger{2})
        .setHelpDescription("Number of parallel jobs."_el);
    commonSet->addOption({"--tag"_el, "tag"_el})
        .setType(el::OptionType::Text)
        .setValueName("label"_el)
        .setMaximum(el::ArgumentCount{8U})
        .setHelpDescription("Free-form label associated with generated records."_el);
    commonSet->addOption({"--cache"_el, "cache"_el}).setHelpDescription("Uses cached data when possible."_el);
    commonSet->addOption({"--no-cache"_el, "no-cache"_el}).setHelpDescription("Ignores cached data."_el);
    module->addSet(commonSet);
}

[[nodiscard]] auto createAcquireModule() -> el::OptionModulePtr {
    auto module = el::OptionModule::create("acquire"_el);
    module->setHelpTitle("Acquire Readings"_el);
    module->setHelpDescription("Takes readings from connected instruments and stores them."_el);
    module->setHelpEpilog("Example: option_help_full acquire --sensor Prism-7 north-bench"_el);
    addCommonModuleOptions(module);

    auto set = el::OptionSet::create();
    set->setHelpTitle("Acquisition"_el);
    set->setHelpDescription("Parameters specific to taking new readings."_el);
    set->addOption({"--sensor"_el, "sensor"_el})
        .setType(el::OptionType::Text)
        .setValueName("sensor"_el)
        .setMaximum(el::ArgumentCount{12U})
        .setHelpDescription(
            "Sensor to query. Can be repeated for campaigns with mixed benches where each instrument exposes "
            "reading families with different latency."_el);
    set->addOption({"--interval"_el, "interval"_el})
        .setType(el::OptionType::Integer)
        .setValueName("seconds"_el)
        .setDefaultValue(el::OptionInteger{30})
        .setHelpDescription("Sampling interval in seconds."_el);
    set->addOption({"--until"_el, "until"_el})
        .setType(el::OptionType::Text)
        .setValueName("time"_el)
        .setHelpDescription("Local time at which to stop acquisition."_el);
    set->addOption({"--sensor-stabilization-window"_el, "sensor-stabilization-window"_el})
        .setType(el::OptionType::Integer)
        .setValueName("seconds"_el)
        .setHelpDescription(
            "Wait time after activating delicate sensors, before accepting readings that may enter automatic "
            "reports."_el);
    set->addOption({"--complete"_el, "complete"_el}).setHelpDescription("Fails if any expected sensor is missing."_el);
    set->addOption("bench"_el)
        .setRequired()
        .setValueName("bench"_el)
        .setHelpDescription("Instrument bench from which to take readings."_el)
        .setHelpVisibility(el::OptionHelpVisibility::Usage);
    module->addSet(set);
    module->setMainFn([](el::OptionValuesPtr) -> el::ExitCode { return el::ExitCode::success(); });
    return module;
}

[[nodiscard]] auto createAnalyzeModule() -> el::OptionModulePtr {
    auto module = el::OptionModule::create("analyze"_el);
    module->setHelpTitle("Analyze Records"_el);
    module->setHelpDescription("Analyzes stored readings and generates summary metrics."_el);
    addCommonModuleOptions(module);

    auto set = el::OptionSet::create();
    set->setHelpTitle("Analysis"_el);
    set->setHelpDescription("Parameters specific to metric calculation."_el);
    set->addOption({"--metric"_el, "metric"_el})
        .addChoice("light"_el)
        .addChoice("temperature"_el)
        .addChoice("humidity"_el)
        .addChoice("vibration"_el)
        .setHelpDescription("Metric family to analyze."_el);
    set->addOption({"--baseline"_el, "baseline"_el})
        .setType(el::OptionType::Text)
        .setHelpDescription("Baseline dataset identifier for comparison."_el);
    set->addOption({"--threshold"_el, "threshold"_el})
        .setType(el::OptionType::Integer)
        .setDefaultValue(el::OptionInteger{10})
        .setHelpDescription("Alert threshold for the selected metric."_el);
    set->addOption({"--explain"_el, "explain"_el}).setHelpDescription("Includes explanatory notes in the report."_el);
    set->addOption("dataset"_el)
        .setRequired()
        .setHelpDescription("Dataset to analyze."_el)
        .setHelpVisibility(el::OptionHelpVisibility::Usage);
    module->addSet(set);
    module->setMainFn([](el::OptionValuesPtr) -> el::ExitCode { return el::ExitCode::success(); });
    return module;
}

[[nodiscard]] auto createExportModule() -> el::OptionModulePtr {
    auto module = el::OptionModule::create("export"_el);
    module->setHelpTitle("Export Report"_el);
    module->setHelpDescription("Exports an instrument report for readers or tools."_el);
    addCommonModuleOptions(module);

    auto set = el::OptionSet::create();
    set->setHelpTitle("Export"_el);
    set->setHelpDescription("Format, language, and destination for the exported report."_el);
    set->addOption({"--format"_el, "format"_el})
        .setChoices(createExportChoices())
        .setDefaultValue("text"_el)
        .setHelpDescription("Output format."_el);
    set->addOption({"--language"_el, "language"_el})
        .addChoice("es"_el)
        .addChoice("en"_el)
        .addChoice("de"_el)
        .setDefaultValue("en"_el)
        .setHelpDescription("Language for generated labels."_el);
    set->addOption({"--audience"_el, "audience"_el})
        .addChoice("class"_el)
        .addChoice("technicians"_el)
        .addChoice("research"_el)
        .setHelpDescription("Audience profile for the report."_el);
    set->addOption({"--output"_el, "output"_el})
        .setType(el::OptionType::Text)
        .setValueName("path"_el)
        .setHelpDescription("Path to the output file."_el);
    set->addOption({"--overwrite"_el, "overwrite"_el}).setHelpDescription("Overwrites an existing output file."_el);
    set->addOption("report"_el)
        .setRequired()
        .setHelpDescription("Identifier of the report to export."_el)
        .setHelpVisibility(el::OptionHelpVisibility::Usage);
    module->addSet(set);
    module->setMainFn([](el::OptionValuesPtr) -> el::ExitCode { return el::ExitCode::success(); });
    return module;
}

/// This large executable registers modules, sets, aliases, defaults, choices, hidden options, and detailed help.
class HelpModuleShowcaseApp final : public el::Application {
public:
    using Application::Application;

protected:
    void initialize() override {
        enableTerminal();
        info().setApplicationName("Prism Atlas"_el);
        info().setApplicationVersion(el::Version{0, 8, 0});
        info().setAuthorName("Erbsland DEV"_el);
        info().setLicenseText("Apache-2.0"_el);
    }
    void registerCommandLineOptions(const el::OptionsPtr &options) override {
        options->setHelpTitle("Prism Atlas"_el);
        options->setHelpDescription("Operates a large command line with scientific-instrument modules."_el);
        options->setHelpEpilog("This executable exists to inspect help and diagnostics from the option system."_el);
        options->addSet(createExecutionSet());
        options->addSet(createReportSet());
        options->addSet(createDisabledLegacySet());
        options->addModule(createAcquireModule());
        options->addModule(createAnalyzeModule());
        options->addModule(createExportModule());
    }
    [[nodiscard]] auto main() -> el::ExitCode override { return el::ExitCode::success(); }
};

}

auto main(const int argc, char *argv[]) -> int {
    auto app = demo::HelpModuleShowcaseApp{argc, argv};
    return app.run();
}
