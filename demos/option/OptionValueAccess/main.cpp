// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoTerminal.hpp>
#include <erbsland/Application.hpp>
#include <erbsland/ApplicationInfo.hpp>
#include <erbsland/ArgumentCount.hpp>
#include <erbsland/ByteIntegerAccess.hpp>
#include <erbsland/CommandLineArguments.hpp>
#include <erbsland/conf/Integer.hpp>
#include <erbsland/ExitCode.hpp>
#include <erbsland/ItemCount.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/OptionInteger.hpp>
#include <erbsland/OptionManager.hpp>
#include <erbsland/OptionModule.hpp>
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

[[nodiscard]] auto createValueOptions() -> el::OptionsPtr {
    auto options = el::Options::create();
    auto info = el::ApplicationInfo{};
    info.setApplicationName("Night Values"_el);
    info.setApplicationVersion(el::Version{0, 8, 0});
    options->setApplicationInfo(info);
    options->addOption({"-v"_el, "--verbose"_el, "verbose"_el})
        .setMaximum(el::ArgumentCount{4U})
        .setHelpTitle("Increases the detail level. May be repeated."_el);
    options->addOption({"-r"_el, "--route"_el, "route"_el})
        .setType(el::OptionType::Text)
        .setMaximum(el::ArgumentCount{3U})
        .setHelpTitle("Route points for the round."_el);
    options->addOption({"--level"_el, "level"_el})
        .setType(el::OptionType::Integer)
        .setDefaultValue(el::OptionInteger{2})
        .setHelpTitle("Static default level."_el);

    auto module = el::OptionModule::create("count"_el);
    module->setHelpTitle("Counts observations in an area."_el);
    module->addOption("area"_el).setRequired().setHelpTitle("Area to count."_el);
    options->addModule(module);
    return options;
}

/// `OptionValues` is the parsed result map produced by `OptionManager`.
///
/// Every command-line spelling and lookup alias for an option points to the same `OptionValue` object.
/// Typed getters read flags, flag counts, integers, text, and repeated value lists.
/// Defaults can come from the option definition or from the getter call, and module-aware tools can inspect
/// `moduleName()` when dispatching manually.
auto optionValueAccess() -> el::ExitCode {
    auto manager = el::OptionManager{createValueOptions()};
    auto arguments = makeArgs(
        {"night-values"_el, "count"_el, "-vv"_el, "--route"_el, "forest-edge"_el, "-r"_el, "pond"_el, "dune"_el});
    const auto values = manager.parseOrThrow(arguments);

    const auto routes = values->getTextList("route"_el);
    auto routeList = el::StringList{};
    for (const auto &route : routes) {
        routeList.append(route.copy());
    }

    el::io::printLine("module: "_el, values->moduleName());
    el::io::printLine("area: "_el, values->getText("area"_el));
    el::io::printLine("verbose count: "_el, values->getFlagCount("verbose"_el));
    el::io::printLine("routes: "_el, routeList.join(", "_el));
    el::io::printLine("level: "_el, values->getInteger("level"_el));
    el::io::printLine("profile: "_el, values->getText("profile"_el, "night"_el));
    el::io::printLine("route argument index: "_el, values->value("route"_el)->argumentIndex().toSizeT());
    return el::ExitCode::success();
}

}

auto main(const int argc, char *argv[]) -> int {
    auto app = el::Application{argc, argv};
    demo::DemoTerminal::enable(app);
    app.setMainFn(demo::optionValueAccess);
    return app.run();
}
