// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/all.hpp>

#include <initializer_list>

namespace demo {

using namespace el::text::literals;

struct ObservationSettings {
    el::String gebied;
    el::String notitie;
    el::OptionInteger interval{0};
    bool stil{false};
};

ObservationSettings settings;

[[nodiscard]] auto makeArgs(std::initializer_list<el::String> args) -> el::CommandLineArguments {
    auto result = el::CommandLineArguments{};
    result.reserve(el::ElementCount{args.size()});
    for (const auto &arg : args) {
        result.append(arg.copy());
    }
    return result;
}

/// Option sets let independent application parts own their command-line definitions and callbacks.
///
/// A pre-parsing callback can adjust a set after configuration is loaded but before arguments are read.
/// A post-parsing callback can copy parsed values into the component that owns the set.
/// Disabled sets are removed from parsing and help, while ordinary help metadata becomes a group in generated help.
[[nodiscard]] auto createObservationSet() -> el::OptionSetPtr {
    auto set = el::OptionSet::create();
    set->setHelpTitle("Observation"_el);
    set->setHelpDescription("Values the field team needs to plan a night round."_el);
    set->addOption({"-g"_el, "--area"_el, "area"_el})
        .setType(el::OptionType::Text)
        .setRequired()
        .setHelpDescription("Name of the observation area."_el);
    set->addOption({"-i"_el, "--interval"_el, "interval"_el})
        .setType(el::OptionType::Integer)
        .setHelpDescription("Minutes between two observations."_el);
    set->setPreParsingFn([](const el::OptionSetPtr &optionSet) -> void {
        optionSet->editOption("interval"_el).setDefaultValue(el::OptionInteger{15});
    });
    set->setPostParsingFn([](const el::OptionValuesPtr &values) -> void {
        settings.gebied = values->getText("area"_el);
        settings.interval = values->getInteger("interval"_el);
    });
    return set;
}

[[nodiscard]] auto createReportSet() -> el::OptionSetPtr {
    auto set = el::OptionSet::create();
    set->setHelpTitle("Report"_el);
    set->setHelpDescription("Values used only by the reporting component."_el);
    set->addOption({"-n"_el, "--note"_el, "note"_el})
        .setType(el::OptionType::Text)
        .setDefaultValue("no observations"_el)
        .setHelpDescription("Short text for the field report."_el);
    set->addOption({"-q"_el, "--quiet"_el, "quiet"_el}).setHelpDescription("Suppresses progress lines."_el);
    set->setPostParsingFn([](const el::OptionValuesPtr &values) -> void {
        settings.notitie = values->getText("note"_el);
        settings.stil = values->getFlag("quiet"_el);
    });
    return set;
}

[[nodiscard]] auto createLegacySet() -> el::OptionSetPtr {
    auto set = el::OptionSet::create();
    set->setHelpTitle("Legacy"_el);
    set->setFlags(el::OptionFlag::Disabled);
    set->addOption({"--old-route"_el, "old-route"_el})
        .setHelpDescription("Deprecated option that is no longer accepted."_el);
    return set;
}

void configureOptions(const el::OptionsPtr &options) {
    options->setHelpTitle("Night Logbook"_el);
    options->setHelpDescription("Combines options from several parts of an observation app."_el);
    options->addSet(createObservationSet());
    options->addSet(createReportSet());
    options->addSet(createLegacySet());
    options->addOption("round"_el).setRequired().setHelpDescription("Name of the planned night round."_el);
}

void printSettings() {
    el::io::printLine("area: "_el, settings.gebied);
    el::io::printLine("interval: "_el, settings.interval);
    el::io::printLine("note: "_el, settings.notitie);
    el::io::printLine("quiet: "_el, el::BooleanFormat::yesNo(), settings.stil);
}

class NightLogbookApp final : public el::Application {
public:
    using Application::Application;

protected:
    void initialize() override {
        enableTerminal();
        info().setApplicationName("Night Logbook"_el);
        info().setApplicationVersion(el::Version{0, 8, 0});
    }
    void registerCommandLineOptions(const el::OptionsPtr &options) override { configureOptions(options); }
    [[nodiscard]] auto main() -> el::ExitCode override {
        printSettings();
        return el::ExitCode::success();
    }
};

auto optionSetCallbacks() -> el::ExitCode {
    auto options = el::Options::create();
    auto info = el::ApplicationInfo{};
    info.setApplicationName("Night Logbook"_el);
    info.setApplicationVersion(el::Version{0, 8, 0});
    options->setApplicationInfo(info);
    options->setExecutablePath("night-logbook"_el);
    configureOptions(options);

    auto manager = el::OptionManager{options};
    const auto values = manager.parseOrThrow(
        makeArgs({"night-logbook"_el, "--area"_el, "duinrand"_el, "--note"_el, "vleermuizen actief"_el, "round-a"_el}));
    if (values == nullptr) {
        return el::ExitCode::success();
    }

    printSettings();
    return el::ExitCode::success();
}

}

auto main(const int argc, char *argv[]) -> int {
    if (argc > 1) {
        auto app = demo::NightLogbookApp{argc, argv};
        return app.run();
    }
    auto app = el::Application{argc, argv};
    app.enableTerminal();
    app.setMainFn(demo::optionSetCallbacks);
    return app.run();
}
