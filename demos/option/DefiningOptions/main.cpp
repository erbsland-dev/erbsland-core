// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/all.hpp>

#include <initializer_list>

namespace demo {

using namespace el::text::literals;

[[nodiscard]] auto makeArgs(std::initializer_list<el::String> args) -> el::CommandLineArguments {
    auto result = el::CommandLineArguments{};
    result.reserve(el::ElementCount{args.size()});
    for (const auto &arg : args) {
        result.append(arg.copy());
    }
    return result;
}

/// An `Option` definition describes names, type, defaults, list limits, choices, validation, and help text.
///
/// A dashed name such as `--instrument` is accepted on the command line.
/// A dashless name on the same regular option, such as `instrument`, is a lookup alias for `OptionValues`.
/// An option with only dashless names is a positional argument.
auto createDefinitionOptions() -> el::OptionsPtr {
    auto options = el::Options::create();
    auto info = el::ApplicationInfo{};
    info.setApplicationName("Optical Planner"_el);
    info.setApplicationVersion(el::Version{0, 8, 0});
    options->setApplicationInfo(info);
    options->setHelpTitle("Optical Planner"_el);
    options->setHelpDescription("Prepares a small measurement sequence for optical instruments."_el);
    options->addOption({"-i"_el, "--instrument"_el, "instrument"_el})
        .setType(el::OptionType::Text)
        .setRequired()
        .setHelpDescription("Instrument used for the sequence."_el);
    options->addOption({"-l"_el, "--level"_el, "level"_el})
        .setType(el::OptionType::Integer)
        .setMaximum(el::ArgumentCount{3U})
        .setHelpDescription("Intensity levels to test. Can be repeated up to three times."_el);
    options->addOption({"-m"_el, "--mode"_el, "mode"_el})
        .addChoice("fast"_el)
        .addChoice("precise"_el)
        .addChoice("night"_el)
        .setDefaultValue("precise"_el)
        .setHelpDescription("Measurement mode for the output."_el);
    options->addOption({"--minimum-signal"_el, "minimum-signal"_el})
        .setType(el::OptionType::Integer)
        .setDefaultValue(el::OptionInteger{3})
        .setValidateFn([](const el::OptionValuePtr &value, el::OptionValuesPtr) -> void {
            if (value->getInteger() < 1 || value->getInteger() > 9) {
                auto context = el::OptionErrorContext{};
                context.setDescription("The signal level must be between 1 and 9."_el);
                throw el::OptionError{context};
            }
        })
        .setHelpDescription("Minimum accepted signal on a scale from 1 to 9."_el);
    options->addOption("sample"_el).setHelpDescription("Sample placed in the optical holder."_el);
    return options;
}

auto definingOptions() -> el::ExitCode {
    auto manager = el::OptionManager{createDefinitionOptions()};
    const auto args = makeArgs(
        {"optica"_el,
            "--instrument"_el,
            "Prisma-7"_el,
            "-l"_el,
            "1"_el,
            "--level=3"_el,
            "--mode=night"_el,
            "vidrio azul"_el});
    const auto values = manager.parseOrThrow(args);

    el::io::printLine("instrument: "_el, values->getText("instrument"_el));
    el::io::printLine("mode: "_el, values->getText("mode"_el));
    el::io::printLine("sample: "_el, values->getText("sample"_el));
    el::io::printLine("levels: "_el, values->getIntegerList("level"_el).size());
    el::io::printLine("minimum signal: "_el, values->getInteger("minimum-signal"_el));
    return el::ExitCode::success();
}

}

auto main(const int argc, char *argv[]) -> int {
    auto app = el::Application{argc, argv};
    app.enableTerminal();
    app.setMainFn(demo::definingOptions);
    return app.run();
}
