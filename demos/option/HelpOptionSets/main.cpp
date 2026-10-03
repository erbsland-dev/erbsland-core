// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoTerminal.hpp>
#include <erbsland/Application.hpp>
#include <erbsland/ArgumentCount.hpp>
#include <erbsland/conf/Integer.hpp>
#include <erbsland/Diagnostic.hpp>
#include <erbsland/Duration.hpp>
#include <erbsland/ExitCode.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/OptionHelpVisibility.hpp>
#include <erbsland/OptionInteger.hpp>
#include <erbsland/Options.hpp>
#include <erbsland/OptionSet.hpp>
#include <erbsland/OptionType.hpp>
#include <erbsland/Path.hpp>
#include <erbsland/pattern/Text.hpp>
#include <erbsland/Version.hpp>

namespace demo {

using namespace el::text::literals;

[[nodiscard]] auto createSamplingSet() -> el::OptionSetPtr {
    auto set = el::OptionSet::create();
    set->setHelpTitle("Sampling"_el);
    set->setHelpDescription("Scope and pace of measurements."_el);
    set->addOption({"-a"_el, "--area"_el, "area"_el})
        .setType(el::OptionType::Text)
        .setValueName("area"_el)
        .setHelpTitle("Laboratory area to study."_el);
    set->addOption({"--line"_el, "line"_el})
        .setType(el::OptionType::Text)
        .setValueName("line"_el)
        .setHelpTitle(
            "Main optical line in the setup, including its bench, guide segment, and reference mirror when the "
            "laboratory uses shared paths."_el);
    set->addOption({"--radius"_el, "radius"_el})
        .setType(el::OptionType::Integer)
        .setValueName("millimeters"_el)
        .setDefaultValue(el::OptionInteger{12})
        .setHelpTitle("Search radius around the stand, in millimeters."_el);
    set->addOption({"--count"_el, "count"_el})
        .setType(el::OptionType::Integer)
        .setDefaultValue(el::OptionInteger{8})
        .setHelpTitle("Number of observation samples."_el);
    set->addOption({"--layer"_el, "layer"_el})
        .setType(el::OptionType::Integer)
        .setValueName("index"_el)
        .setMaximum(el::ArgumentCount{4U})
        .setHelpTitle("Data layer to include. Can be repeated."_el);
    set->addOption({"--shift"_el, "shift"_el})
        .addChoice("morning"_el)
        .addChoice("afternoon"_el)
        .addChoice("night"_el)
        .setHelpTitle("Laboratory shift for the measurement."_el);
    set->addOption({"--environment"_el, "environment"_el})
        .addChoice("dry"_el)
        .addChoice("stable"_el)
        .addChoice("vibration"_el)
        .setValueName("condition"_el)
        .setHelpTitle("Environmental condition observed during sampling."_el);
    set->addOption({"--extended-thermal-drift-correction"_el, "extended-thermal-drift-correction"_el})
        .setType(el::OptionType::Integer)
        .setValueName("minutes"_el)
        .setHelpTitle(
            "Duration used to compensate for slow thermal drift before fixing reference samples on benches exposed to "
            "ventilation changes."_el);
    set->addOption({"--include-spare"_el, "include-spare"_el})
        .setHelpTitle("Includes spare sensors in the reading."_el);
    set->addOption({"--trial"_el, "trial"_el}).setHelpTitle("Validates sampling without writing results."_el);
    return set;
}

[[nodiscard]] auto createOutputSet() -> el::OptionSetPtr {
    auto set = el::OptionSet::create();
    set->setHelpTitle("Output"_el);
    set->setHelpDescription("Report format, files, and labels."_el);
    set->addOption({"-o"_el, "--output"_el, "output"_el})
        .setType(el::OptionType::Text)
        .setValueName("path"_el)
        .setHelpTitle("Path to the output file."_el);
    set->addOption({"--format"_el, "format"_el})
        .addChoice("text"_el)
        .addChoice("json"_el)
        .addChoice("table"_el)
        .setValueName("format"_el)
        .setDefaultValue("text"_el)
        .setHelpTitle("Report format."_el);
    set->addOption({"--theme"_el, "theme"_el})
        .addChoice("light"_el)
        .addChoice("dark"_el)
        .addChoice("ink"_el)
        .setHelpTitle("Visual theme for the report."_el);
    set->addOption({"--language"_el, "language"_el})
        .addChoice("es"_el)
        .addChoice("en"_el)
        .addChoice("de"_el)
        .setDefaultValue("en"_el)
        .setHelpTitle("Natural language for labels."_el);
    set->addOption({"--title"_el, "title"_el})
        .setType(el::OptionType::Text)
        .setValueName("text"_el)
        .setHelpTitle("Report title."_el);
    set->addOption({"--subtitle"_el, "subtitle"_el})
        .setType(el::OptionType::Text)
        .setValueName("text"_el)
        .setHelpTitle(
            "Subtitle printed below the title, used to distinguish repeated campaigns that share instruments and "
            "review dates."_el);
    set->addOption({"--width"_el, "width"_el})
        .setType(el::OptionType::Integer)
        .setDefaultValue(el::OptionInteger{80})
        .setHelpTitle("Preferred width for text tables."_el);
    set->addOption({"--no-color"_el, "no-color"_el}).setHelpTitle("Disables color in the output."_el);
    set->addOption({"--overwrite"_el, "overwrite"_el}).setHelpTitle("Overwrites an existing file."_el);
    return set;
}

[[nodiscard]] auto createInstrumentSet() -> el::OptionSetPtr {
    auto set = el::OptionSet::create();
    set->setHelpTitle("Instruments"_el);
    set->setHelpDescription("Sensors and calibration profiles."_el);
    set->addOption({"--sensor"_el, "sensor"_el})
        .setType(el::OptionType::Text)
        .setValueName("sensor"_el)
        .setMaximum(el::ArgumentCount{6U})
        .setHelpTitle(
            "Sensor to read. Can be repeated to compare primary instruments, control replicas, and auxiliary "
            "channels in the same execution."_el);
    set->addOption({"--temperature"_el, "temperature"_el}).setHelpTitle("Includes temperature readings."_el);
    set->addOption({"--humidity"_el, "humidity"_el}).setHelpTitle("Includes humidity readings."_el);
    set->addOption({"--light"_el, "light"_el}).setHelpTitle("Includes light readings."_el);
    set->addOption({"--sound"_el, "sound"_el}).setHelpTitle("Includes acoustic readings."_el);
    set->addOption({"--interval"_el, "interval"_el})
        .setType(el::OptionType::Integer)
        .setValueName("seconds"_el)
        .setDefaultValue(el::OptionInteger{60})
        .setHelpTitle("Sampling interval in seconds."_el);
    set->addOption({"--window"_el, "window"_el})
        .setType(el::OptionType::Integer)
        .setDefaultValue(el::OptionInteger{15})
        .setHelpTitle("Aggregation window in minutes."_el);
    set->addOption({"--calibration"_el, "calibration"_el})
        .addChoice("none"_el)
        .addChoice("field"_el)
        .addChoice("laboratory"_el)
        .setValueName("profile"_el)
        .setHelpTitle("Calibration profile applied to the sensors."_el);
    set->addOption({"--extended-laboratory-calibration-profile"_el, "extended-laboratory-calibration-profile"_el})
        .setType(el::OptionType::Text)
        .setValueName("profile"_el)
        .setHelpTitle(
            "Extended calibration profile loaded from the laboratory registry for sensors that require nonlinear "
            "curves and subsequent manual review."_el);
    return set;
}

[[nodiscard]] auto createDiagnosticSet() -> el::OptionSetPtr {
    auto set = el::OptionSet::create();
    set->setHelpTitle("Diagnostics"_el);
    set->setHelpDescription("Validation, safety, and internal traces."_el);
    set->addOption({"--strict"_el, "strict"_el}).setHelpTitle("Rejects incomplete observations."_el);
    set->addOption({"--allow-estimates"_el, "allow-estimates"_el})
        .setHelpTitle("Allows values estimated by secondary instruments."_el);
    set->addOption({"--max-pause"_el, "max-pause"_el})
        .setType(el::OptionType::Integer)
        .setValueName("seconds"_el)
        .setDefaultValue(el::OptionInteger{5})
        .setHelpTitle("Maximum accepted pause between samples."_el);
    set->addOption({"--retries"_el, "retries"_el})
        .setType(el::OptionType::Integer)
        .setDefaultValue(el::OptionInteger{2})
        .setHelpTitle("Retries for transient instrument errors."_el);
    set->addOption({"--log"_el, "log"_el})
        .addChoice("error"_el)
        .addChoice("info"_el)
        .addChoice("debug"_el)
        .setValueName("level"_el)
        .setHelpTitle("Diagnostic log level."_el);
    set->addOption({"--trace-parser"_el, "trace-parser"_el})
        .setHelpTitle("Traces internal decisions by the option parser."_el)
        .setHelpVisibility(el::OptionHelpVisibility::Hidden);
    return set;
}

/// This executable registers several logical option sets for help rendering tests.
class HelpOptionSetsApp final : public el::Application {
public:
    using Application::Application;

protected:
    void initialize() override {
        DemoTerminal::enable(*this);
        info().setApplicationName("Prism Notebook"_el);
        info().setApplicationVersion(el::Version{0, 8, 0});
        info().setAuthorName("Erbsland DEV"_el);
        info().setLicenseText("Apache-2.0"_el);
    }
    void registerCommandLineOptions(const el::OptionsPtr &options) override {
        options->setHelpTitle("Prism Notebook"_el);
        options->setHelpDescription("Records a detailed notebook for a scientific-instrument bench."_el);
        options->setHelpEpilog("This executable exists to inspect groups, ordering, and line wrapping."_el);
        options->addSet(createSamplingSet());
        options->addSet(createOutputSet());
        options->addSet(createInstrumentSet());
        options->addSet(createDiagnosticSet());
        options->addOption("project"_el)
            .setRequired()
            .setValueName("project"_el)
            .setHelpTitle("Identifier of the laboratory project."_el)
            .setHelpVisibility(el::OptionHelpVisibility::Usage);
    }
    [[nodiscard]] auto main() -> el::ExitCode override { return el::ExitCode::success(); }
};

}

auto main(const int argc, char *argv[]) -> int {
    auto app = demo::HelpOptionSetsApp{argc, argv};
    return app.run();
}
