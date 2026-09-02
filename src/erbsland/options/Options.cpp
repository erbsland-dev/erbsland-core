// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Options.hpp"

#include "OptionFlag.hpp"
#include "OptionModule.hpp"
#include "OptionSet.hpp"
#include "OptionType.hpp"

#include "impl/ExecutableName.hpp"

#include "../text/Literals.hpp"

#include <memory>
#include <utility>

namespace erbsland::options {

using namespace text::literals;

Options::Options() : _builtInOptionSet{createBuiltInOptionSet()} {
}

auto Options::create() -> OptionsPtr {
    return std::make_shared<Options>();
}

void Options::addSet(OptionSetPtr optionSet) {
    _optionSets.emplace_back(std::move(optionSet));
}

void Options::addModule(OptionModulePtr optionModule) {
    _optionModules.emplace_back(std::move(optionModule));
}

void Options::setExecutablePath(text::String executablePath) {
    _executablePath = std::move(executablePath);
    _executableName = impl::extractExecutableName(_executablePath);
}

auto Options::addOption(std::initializer_list<text::String> names) -> OptionEditor {
    return defaultOptionSet()->addOption(names);
}

auto Options::editOption(const text::String &name) -> OptionEditor {
    for (const auto &optionSet : _optionSets) {
        auto editor = optionSet->editOption(name);
        if (editor.isValid()) {
            return editor;
        }
    }
    return {};
}

auto Options::defaultOptionSet() -> OptionSetPtr {
    if (_optionSets.empty()) {
        _optionSets.emplace_back(OptionSet::create());
    }
    return _optionSets.front();
}

auto Options::createBuiltInOptionSet() -> OptionSetPtr {
    auto optionSet = OptionSet::create();
    optionSet->addOption({"-h"_el, "--help"_el})
        .setType(OptionType::Text)
        .setFlag(OptionFlag::AcceptAsFlag)
        .setHelp("Display this help."_el)
        .setHelpVisibility(OptionHelpVisibility::Overview);
    optionSet->addOption("--version"_el)
        .setHelp("Display version information."_el)
        .setHelpVisibility(OptionHelpVisibility::Overview);
    return optionSet;
}

}
