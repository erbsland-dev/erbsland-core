// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoTerminal.hpp>
#include <erbsland/Application.hpp>
#include <erbsland/ApplicationInfo.hpp>
#include <erbsland/BooleanFormat.hpp>
#include <erbsland/ByteIntegerAccess.hpp>
#include <erbsland/CommandLineArguments.hpp>
#include <erbsland/conf/Integer.hpp>
#include <erbsland/ExitCode.hpp>
#include <erbsland/ItemCount.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/OptionInteger.hpp>
#include <erbsland/OptionManager.hpp>
#include <erbsland/OptionResult.hpp>
#include <erbsland/OptionResultStatus.hpp>
#include <erbsland/Options.hpp>
#include <erbsland/OptionType.hpp>
#include <erbsland/OptionValues.hpp>
#include <erbsland/pattern/Text.hpp>
#include <erbsland/StandardStreams.hpp>
#include <erbsland/String.hpp>
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

/// Use `OptionManager` directly when option parsing is only one part of a larger startup flow.
///
/// Manual parsing returns an `OptionResult` instead of immediately exiting or throwing.
/// This is useful for libraries, test tools, embedded command interpreters, or applications that need to route help,
/// version, and error documents through their own output system.
auto createManualOptions() -> el::OptionsPtr {
    auto options = el::Options::create();
    auto info = el::ApplicationInfo{};
    info.setApplicationName("Photometry Notebook"_el);
    info.setApplicationVersion(el::Version{0, 8, 0});
    options->setApplicationInfo(info);
    options->setHelpTitle("Photometry Notebook"_el);
    options->setHelpDescription("Records a short note from a photometer measurement."_el);
    options->addOption({"-q"_el, "--quiet"_el, "quiet"_el}).setHelpTitle("Reduces progress output."_el);
    options->addOption({"-n"_el, "--note"_el, "note"_el})
        .setType(el::OptionType::Text)
        .setRequired()
        .setHelpTitle("Text of the laboratory note."_el);
    options->addOption({"-r"_el, "--repeat"_el, "repeat"_el})
        .setType(el::OptionType::Integer)
        .setDefaultValue(el::OptionInteger{1})
        .setHelpTitle("Number of times to repeat the note in the report."_el);
    return options;
}

auto manualParsing() -> el::ExitCode {
    auto manager = el::OptionManager{createManualOptions()};
    auto args = makeArgs({"fotometria"_el, "--note"_el, "Lectura estable en lámpara azul"_el, "--repeat=2"_el});
    const auto result = manager.parse(args);

    if (result.status() != el::OptionResultStatus::Success) {
        if (result.status() == el::OptionResultStatus::DisplayHelp) {
            el::io::printLine(manager.helpDocument(result.values()->moduleName()).toString());
        } else if (result.status() == el::OptionResultStatus::DisplayVersion) {
            el::io::printLine(manager.versionDocument(result.values()->moduleName()).toString());
        } else if (result.errorContext().has_value()) {
            el::io::printLine(manager.errorDocument(result.errorContext().value()).toString());
        }
        return el::ExitCode::failure();
    }

    const auto values = result.values();
    el::io::printLine("note: "_el, values->getText("note"_el));
    el::io::printLine("repetitions: "_el, values->getInteger("repeat"_el));
    el::io::printLine("quiet: "_el, el::BooleanFormat::yesNo(), values->getFlag("quiet"_el));
    return el::ExitCode::success();
}

}

auto main(const int argc, char *argv[]) -> int {
    auto app = el::Application{argc, argv};
    demo::DemoTerminal::enable(app);
    app.setMainFn(demo::manualParsing);
    return app.run();
}
