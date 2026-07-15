// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/all.hpp>

#include <initializer_list>

namespace demo {

using namespace el::text::literals;

struct LaboratorySettings {
    bool quiet{false};
    el::StringView instrument;
    el::StringView reportTitle;
    el::OptionInteger sampleCount{0};
};

LaboratorySettings appSettings;

[[nodiscard]] auto makeArgs(std::initializer_list<el::StringView> args) -> el::CommandLineArguments {
    auto result = el::CommandLineArguments{};
    result.reserve(el::ElementCount{args.size()});
    for (const auto &arg : args) {
        result.append(arg.copy());
    }
    return result;
}

/// Use `OptionSet` to let each component own its command line options and callbacks.
///
/// Sets are registered together for one parse operation, but their callbacks stay separate.
/// A set can prepare its definitions in a pre-parsing callback and can read all parsed values in a post-parsing
/// callback, including values from other sets.
auto createInstrumentSet() -> el::OptionSetPtr {
    auto instrumentSet = el::OptionSet::create();
    instrumentSet->setHelpTitle("Instrument"_el);
    instrumentSet->setHelpDescription("Options for the component that selects the instrument."_el);
    instrumentSet->addOption({"-q"_el, "--quiet"_el, "quiet"_el}).setHelpDescription("Reduces progress output."_el);
    instrumentSet->addOption({"-i"_el, "--instrument"_el, "instrument"_el})
        .setType(el::OptionType::Text)
        .setRequired()
        .setHelpDescription("Name of the laboratory instrument."_el);
    instrumentSet->setPostParsingFn([](const el::OptionValuesPtr &values) -> void {
        appSettings.quiet = values->getFlag("quiet"_el);
        appSettings.instrument = values->getText("instrument"_el);
    });
    return instrumentSet;
}

auto createReportSet() -> el::OptionSetPtr {
    auto reportSet = el::OptionSet::create();
    reportSet->setHelpTitle("Report"_el);
    reportSet->setHelpDescription("Options for the component that writes the report."_el);
    reportSet->addOption({"-t"_el, "--title"_el, "title"_el})
        .setType(el::OptionType::Text)
        .setDefaultValue(el::String{"Calibration report"_el})
        .setHelpDescription("Title of the generated report."_el);
    reportSet->addOption({"-s"_el, "--samples"_el, "samples"_el})
        .setType(el::OptionType::Integer)
        .setDefaultValue(el::OptionInteger{4})
        .setHelpDescription("Number of samples to include."_el);
    reportSet->setPostParsingFn([](const el::OptionValuesPtr &values) -> void {
        appSettings.reportTitle = values->getText("title"_el);
        appSettings.sampleCount = values->getInteger("samples"_el);
    });
    return reportSet;
}

auto optionSets() -> el::ExitCode {
    auto options = el::Options::create();
    auto info = el::ApplicationInfo{};
    info.setApplicationName("Instrument Report"_el);
    info.setApplicationVersion(el::Version{0, 8, 0});
    options->setApplicationInfo(info);
    options->setHelpTitle("Instrument Report"_el);
    options->setHelpDescription("Builds a short report from independent option sets."_el);
    options->addSet(createInstrumentSet());
    options->addSet(createReportSet());

    auto manager = el::OptionManager{options};
    const auto values = manager.parseOrThrow(
        makeArgs({"instrumentos"_el, "--instrument"_el, "Interferómetro Norte"_el, "--title"_el, "Prueba matinal"_el}));
    if (values == nullptr) {
        return el::ExitCode::success();
    }

    el::io::printLine("instrument: "_el, appSettings.instrument);
    el::io::printLine("title: "_el, appSettings.reportTitle);
    el::io::printLine("samples: "_el, appSettings.sampleCount);
    el::io::printLine("quiet: "_el, el::BooleanFormat::yesNo(), appSettings.quiet);
    return el::ExitCode::success();
}

}

auto main(const int argc, char *argv[]) -> int {
    auto app = el::Application{argc, argv};
    app.enableTerminal();
    app.setMainFn(demo::optionSets);
    return app.run();
}
