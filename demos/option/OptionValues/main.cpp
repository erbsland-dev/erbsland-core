// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoTerminal.hpp>
#include <erbsland/Application.hpp>
#include <erbsland/ApplicationInfo.hpp>
#include <erbsland/ArgumentCount.hpp>
#include <erbsland/BooleanFormat.hpp>
#include <erbsland/ByteIntegerAccess.hpp>
#include <erbsland/CommandLineArguments.hpp>
#include <erbsland/conf/Integer.hpp>
#include <erbsland/ExitCode.hpp>
#include <erbsland/ItemCount.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/OptionInteger.hpp>
#include <erbsland/OptionManager.hpp>
#include <erbsland/Options.hpp>
#include <erbsland/OptionType.hpp>
#include <erbsland/OptionValue.hpp>
#include <erbsland/OptionValues.hpp>
#include <erbsland/pattern/Text.hpp>
#include <erbsland/StandardStreams.hpp>
#include <erbsland/String.hpp>
#include <erbsland/StringList.hpp>
#include <erbsland/Version.hpp>

#include <initializer_list>

namespace demo {

using namespace el::text::literals;

[[nodiscard]] auto makeArgs(std::initializer_list<el::String> args) -> el::CommandLineArguments {
    auto result = el::CommandLineArguments{};
    result.reserve(el::ItemCount::fromInteger(args.size()));
    for (const auto &arg : args) {
        result.append(arg.copy());
    }
    return result;
}

/// `OptionValues` stores parsed values under every accepted name of an option.
///
/// Command line spellings and dashless lookup aliases point to the same `OptionValue` instance.
/// Typed getters return flags, flag counts, integers, text, and lists without forcing application code to inspect the
/// underlying variant directly.
auto optionValues() -> el::ExitCode {
    auto options = el::Options::create();
    auto info = el::ApplicationInfo{};
    info.setApplicationName("Laboratory Values"_el);
    info.setApplicationVersion(el::Version{0, 8, 0});
    options->setApplicationInfo(info);
    options->addOption({"-v"_el, "--verbose"_el, "verbose"_el}).setHelpTitle("Increases the detail level."_el);
    options->addOption({"-p"_el, "--point"_el, "point"_el})
        .setType(el::OptionType::Text)
        .setMaximum(el::ArgumentCount{4U})
        .setHelpTitle("Measurement point. Can appear up to four times."_el);
    options->addOption({"--level"_el, "level"_el})
        .setType(el::OptionType::Integer)
        .setDefaultValue(el::OptionInteger{2})
        .setHelpTitle("Default observation level."_el);
    options->addOption("instrument"_el).setRequired().setHelpTitle("Primary instrument."_el);

    auto manager = el::OptionManager{options};
    auto arguments =
        makeArgs({"valores"_el, "-vv"_el, "--point"_el, "entrada"_el, "-p"_el, "lente"_el, "microscopio"_el});
    const auto values = manager.parseOrThrow(arguments);

    const auto verboseByLongName = values->value("--verbose"_el);
    const auto verboseByAlias = values->value("verbose"_el);
    const auto points = values->getTextList("point"_el);
    el::io::printLine("same object: "_el, el::BooleanFormat::yesNo(), verboseByLongName == verboseByAlias);
    el::io::printLine("verbose count: "_el, values->getFlagCount("verbose"_el));
    el::io::printLine("instrument: "_el, values->getText("instrument"_el));
    el::io::printLine("level: "_el, values->getInteger("level"_el));
    auto pointList = el::StringList{};
    for (const auto &point : points) {
        pointList.append(point.copy());
    }
    el::io::printLine("points: "_el, pointList.join(", "_el));
    el::io::printLine("first index: "_el, values->value("point"_el)->argumentIndex().toSizeT());
    return el::ExitCode::success();
}

}

auto main(const int argc, char *argv[]) -> int {
    auto app = el::Application{argc, argv};
    demo::DemoTerminal::enable(app);
    app.setMainFn(demo::optionValues);
    return app.run();
}
