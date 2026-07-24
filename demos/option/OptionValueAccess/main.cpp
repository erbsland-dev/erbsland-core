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

[[nodiscard]] auto createValueOptions() -> el::OptionsPtr {
    auto options = el::Options::create();
    auto info = el::ApplicationInfo{};
    info.setApplicationName("Night Values"_el);
    info.setApplicationVersion(el::Version{0, 8, 0});
    options->setApplicationInfo(info);
    options->addOption({"-v"_el, "--verbose"_el, "verbose"_el})
        .setMaximum(el::ArgumentCount{4U})
        .setHelpDescription("Increases the detail level. May be repeated."_el);
    options->addOption({"-r"_el, "--route"_el, "route"_el})
        .setType(el::OptionType::Text)
        .setMaximum(el::ArgumentCount{3U})
        .setHelpDescription("Route points for the round."_el);
    options->addOption({"--level"_el, "level"_el})
        .setType(el::OptionType::Integer)
        .setDefaultValue(el::OptionInteger{2})
        .setHelpDescription("Static default level."_el);

    auto module = el::OptionModule::create("count"_el);
    module->setHelpDescription("Counts observations in an area."_el);
    module->addOption("area"_el).setRequired().setHelpDescription("Area to count."_el);
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
    app.enableTerminal();
    app.setMainFn(demo::optionValueAccess);
    return app.run();
}
